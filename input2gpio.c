#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define EVENT_DEV "/dev/input/event1"

#define RDA_CONFIG_REGS 0x11a09000

volatile struct config_regs {
  uint32_t chip_id;
  uint32_t build_version;
  uint32_t bb_gpio_mode;
  uint32_t ap_gpio_a_mode;
  uint32_t ap_gpio_b_mode;
  uint32_t ap_gpio_d_mode;
  // ...
}* config_regs;

#define GPIOA_BASE 0x20930000

volatile struct gpio_regs {
  uint32_t oen_val;
  uint32_t oen_set_out;
  uint32_t oen_set_in;
  uint32_t val;
  uint32_t set;
  uint32_t clr;
  // ...
}* gpioa;

enum jamma_key {
  JKEY_COIN,
  JKEY_START,
  JKEY_UP,
  JKEY_DOWN,
  JKEY_LEFT,
  JKEY_RIGHT,
  JKEY_1,
  JKEY_2,
  JKEY_3,
};

int gpio_pin[] = { 0, 1, 2, 3, 4, 5, 6, 14, 15 };

enum direction {
  DIR_X,
  DIR_Y,
};

struct dpad {
  uint16_t code;
  int32_t center;
  int32_t range;
  enum direction direction;
};

struct button {
  uint16_t code;
  enum jamma_key key;
};

struct gamepad {
  const char* name;
  struct dpad dpads[4];
  struct button buttons[5];
};

struct gamepad gamepads[] = {
  {
    .name = "HORI CO.,LTD. HORIPAD 4 ",
    .dpads = {
      { .code = 0x00, .center = 128, .range = 16, .direction = DIR_X },
      { .code = 0x01, .center = 128, .range = 16, .direction = DIR_Y },
      { .code = 0x10, .center =   0, .range =  0, .direction = DIR_X },
      { .code = 0x11, .center =   0, .range =  0, .direction = DIR_Y }
    },
    .buttons = {
      { .code = 0x138, .key = JKEY_COIN  },
      { .code = 0x139, .key = JKEY_START },
      { .code = 0x130, .key = JKEY_1     },
      { .code = 0x131, .key = JKEY_2     },
      { .code = 0x132, .key = JKEY_3     }
    }
  },
  {
    .name = "Xbox Gamepad (userspace driver)",
    .dpads = {
      { .code = 0x10, .center = 0, .range = 0, .direction = DIR_X },
      { .code = 0x11, .center = 0, .range = 0, .direction = DIR_Y },
    },
    .buttons = {
      { .code = 0x13a, .key = JKEY_COIN  },
      { .code = 0x13b, .key = JKEY_START },
      { .code = 0x130, .key = JKEY_1     },
      { .code = 0x131, .key = JKEY_2     },
      { .code = 0x133, .key = JKEY_3     },
    }
  }
};

void update(enum jamma_key key, int value) {
#if 0
  static int state = 0;
  state &= ~(1 << key);
  if (value)
    state |= 1 << key;
  for (int i = 8; i >= 0; --i)
    fprintf(stderr, "%d", (state >> i) & 1);
  fprintf(stderr, "\n");
#endif

  int pin = gpio_pin[key];
  if (value)
    gpioa->clr = (1 << pin);
  else
    gpioa->set = (1 << pin);
}

void do_bridge(int fd) {
  char name[256];
  ioctl(fd, EVIOCGNAME(sizeof(name)), name);

  struct gamepad* detected_gamepad = NULL;
  for (int i = 0; i < sizeof(gamepads) / sizeof(struct gamepad); ++i) {
    if (0 == strcmp(name, gamepads[i].name)) {
      detected_gamepad = &gamepads[i];
      fprintf(stderr, "known devide detected: %s\n", name);
      break;
    }
  }
  if (!detected_gamepad)
    fprintf(stderr, "unknown device detected: %s\n", name);

  for (;;) {
    struct input_event e;
    int size = read(fd, &e, sizeof(e));
    if (size == -1 && errno == EAGAIN)
      continue;
    if (size != sizeof(e))
      return;
    if (detected_gamepad) {
      int consumed = 0;
      if (e.type == EV_KEY) {
        for (int i = 0; i < sizeof(detected_gamepad->buttons) / sizeof(struct button); ++i) {
          if (detected_gamepad->buttons[i].code != e.code)
            continue;
          consumed = 1;
          update(detected_gamepad->buttons[i].key, e.value);
          break;
        }
      } else if (e.type == EV_ABS) {
        for (int i = 0; i < sizeof(detected_gamepad->dpads) / sizeof(struct dpad); ++i) {
          if (detected_gamepad->dpads[i].code != e.code)
            continue;
          consumed = 1;
          if ((detected_gamepad->dpads[i].center - detected_gamepad->dpads[i].range) > e.value) {
            update(detected_gamepad->dpads[i].direction == DIR_X ? JKEY_RIGHT : JKEY_DOWN, 0);
            update(detected_gamepad->dpads[i].direction == DIR_X ? JKEY_LEFT : JKEY_UP, 1);
          } else if ((detected_gamepad->dpads[i].center + detected_gamepad->dpads[i].range) < e.value) {
            update(detected_gamepad->dpads[i].direction == DIR_X ? JKEY_LEFT : JKEY_UP, 0);
            update(detected_gamepad->dpads[i].direction == DIR_X ? JKEY_RIGHT : JKEY_DOWN, 1);
          } else {
            update(detected_gamepad->dpads[i].direction == DIR_X ? JKEY_LEFT : JKEY_UP, 0);
            update(detected_gamepad->dpads[i].direction == DIR_X ? JKEY_RIGHT : JKEY_DOWN, 0);
          }
          break;
        }
      }
      if (consumed)
        continue;
    }
    if (e.type == EV_KEY || e.type == EV_ABS)
      printf("%04x: %04x, %d\n", e.type, e.code, e.value);
  }
}

int main(int argc, char** argv) {
  int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (mem_fd < 0) {
    perror("can not open /dev/mem");
    exit(EXIT_FAILURE);
  }

  int prot = PROT_READ | PROT_WRITE;
  size_t size = sysconf(_SC_PAGE_SIZE);
  config_regs = mmap(NULL, size, prot, MAP_SHARED, mem_fd, RDA_CONFIG_REGS);
  gpioa = mmap(NULL, size, prot, MAP_SHARED, mem_fd, GPIOA_BASE);

  // Enable GPIO_A 14 and 15 in addition to GPIO_A 0 through 6.
  config_regs->ap_gpio_a_mode |= (1 << 14) | (1 << 15);

  // Set GPIO_A 0...6, 14, and 15 as output port, and drive HIGH signals.
  gpioa->oen_set_out = 0xc07f;
  gpioa->set = 0xc07f;

  for (;;) {
    int fd = open(EVENT_DEV, O_RDWR);
    if (fd >= 0) {
      do_bridge(fd);
      close(fd);
    }
    sleep(1);
  }
  return 0;
}
