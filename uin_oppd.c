// Copyright 2017 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#define GPIO_PA_BASE 0x01c20000
#define GPIO_PA_OFFSET 0x0800

#define B_BACK  (1 << 12)
#define B_START (1 << 11)
#define B_UP    (1 <<  6)
#define B_DOWN  (1 <<  1)
#define B_LEFT  (1 <<  0)
#define B_RIGHT (1 <<  3)
#define B_A     (1 << 19)
#define B_B     (1 <<  7)
#define B_X     (1 <<  8)
#define B_Y     (1 <<  9)
#define B_RT    (1 << 10)
#define B_LT    (1 << 20)
#define B_RB    (1 << 13)
#define B_LB    (1 << 14)
#define B_XBOX  (1 <<  2)

#define B_DPAD (B_UP | B_DOWN | B_LEFT | B_RIGHT)
#define B_ALL (B_BACK | B_START | B_DPAD | \
    B_A | B_B | B_X | B_Y | B_RT | B_LT | B_RB | B_LB | B_XBOX)

#define MODE_UINPUT 0
#define MODE_KBD 1
#define MODE_HID 2
int mode = MODE_UINPUT;

int rapid_a = 0;
int rapid_b = 0;
int rapid_x = 0;
int rapid_y = 0;

int rapid_count = 0;

struct pio_cfg {
  volatile uint32_t cfg[4];
  volatile uint32_t dat;
  volatile uint32_t drv[2];
  volatile uint32_t pul[2];
}* gpio;

int led_map[4] = { 2, 0, 3, 1 };

void led_off(int led) {
  int n = led_map[led];
  gpio[2].cfg[0] &= ~(0xf << (n * 4));
  gpio[2].pul[0] &= ~(0x3 << (n * 2));
}

void led_on(int led) {
  int n = led_map[led];
  gpio[2].cfg[0] |= (0x1 << (n * 4));
  gpio[2].dat &= ~(1 << n);
}

void led_set(int n, int on) {
  if (on)
    led_on(n);
  else
    led_off(n);
}

int gpio_setup() {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0)
    return fd;

  int prot = PROT_READ | PROT_WRITE;
  size_t size = sysconf(_SC_PAGE_SIZE);
  char *gpio_base = mmap(NULL, size, prot, MAP_SHARED, fd, GPIO_PA_BASE);
  gpio = (struct pio_cfg*)&gpio_base[GPIO_PA_OFFSET];

  gpio[0].cfg[0] &= 0x00ff0000;  // PA07-PA00: input [7:6,3:0]
  gpio[0].cfg[1] &= 0xf0000000;  // PA15-PA08: input [14:8]
  gpio[0].cfg[2] &= 0xfff00fff;  // PA21-PA16: input [20:19]
  gpio[0].pul[0] &= 0xc0000f00;  // PA15-PA00: pull-* disabled [14:6,3:0]
  gpio[0].pul[0] |= 0x15555055;  // PA15-PA00: pull-up [14:6,3:0]
  gpio[0].pul[1] &= 0xfffffc3f;  // PA21-PA16: pull-* disabled [20:19]
  gpio[0].pul[1] |= 0x00000140;  // PA21-PA16: pull-up [20:19]

  led_on(0);
  led_off(1);
  led_off(2);
  led_off(3);

  return 0;
}

int js_setup() {
  int fd = open("/dev/uinput", O_WRONLY);
  if (fd < 0)
    return fd;

  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, BTN_A);       // A
  ioctl(fd, UI_SET_KEYBIT, BTN_B);       // B
  ioctl(fd, UI_SET_KEYBIT, BTN_X);       // X
  ioctl(fd, UI_SET_KEYBIT, BTN_Y);       // Y
  ioctl(fd, UI_SET_KEYBIT, BTN_TL);      // LB
  ioctl(fd, UI_SET_KEYBIT, BTN_TR);      // RB
  ioctl(fd, UI_SET_KEYBIT, BTN_TL2);     // LT
  ioctl(fd, UI_SET_KEYBIT, BTN_TR2);     // RT
  ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);  // BACK
  ioctl(fd, UI_SET_KEYBIT, BTN_START);   // START
  ioctl(fd, UI_SET_KEYBIT, BTN_MODE);    // Xbox
  ioctl(fd, UI_SET_KEYBIT, BTN_THUMBL);
  ioctl(fd, UI_SET_KEYBIT, BTN_THUMBR);

  if (mode == MODE_KBD) {
    ioctl(fd, UI_SET_KEYBIT, KEY_A);      // X
    ioctl(fd, UI_SET_KEYBIT, KEY_D);
    ioctl(fd, UI_SET_KEYBIT, KEY_L);
    ioctl(fd, UI_SET_KEYBIT, KEY_Q);      // L
    ioctl(fd, UI_SET_KEYBIT, KEY_R);
    ioctl(fd, UI_SET_KEYBIT, KEY_S);      // Y
    ioctl(fd, UI_SET_KEYBIT, KEY_U);
    ioctl(fd, UI_SET_KEYBIT, KEY_X);      // B
    ioctl(fd, UI_SET_KEYBIT, KEY_W);      // R
    ioctl(fd, UI_SET_KEYBIT, KEY_Z);      // A
    ioctl(fd, UI_SET_KEYBIT, KEY_ENTER);  // SELECT
    ioctl(fd, UI_SET_KEYBIT, KEY_SPACE);  // START
    ioctl(fd, UI_SET_KEYBIT, KEY_ESC);    // LB
    ioctl(fd, UI_SET_KEYBIT, KEY_UP);
    ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);
  } else {
    ioctl(fd, UI_SET_KEYBIT, KEY_U);
    ioctl(fd, UI_SET_KEYBIT, KEY_D);
    ioctl(fd, UI_SET_KEYBIT, KEY_L);
    ioctl(fd, UI_SET_KEYBIT, KEY_R);
  }

  ioctl(fd, UI_SET_EVBIT, EV_ABS);
  ioctl(fd, UI_SET_ABSBIT, ABS_X);
  ioctl(fd, UI_SET_ABSBIT, ABS_Y);
  ioctl(fd, UI_SET_ABSBIT, ABS_RX);
  ioctl(fd, UI_SET_ABSBIT, ABS_RY);
  ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);    // Stick L/R [-32767, 32767]
  ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);    // Stick U/D [-32767, 32767]

  // for kernel 3.4.113-sun8i (Orange Pi)
  struct uinput_user_dev setup;
  memset(&setup, 0, sizeof(setup));
  snprintf(setup.name, UINPUT_MAX_NAME_SIZE,
      "Hori Fighting Stick EX2 (embedded)");
  int i;
  for (i = 0; i < ABS_CNT;   ++i) {
    setup.absmax[i] =  32767;
    setup.absmin[i] = -32767;
  }
  write(fd, &setup, sizeof(setup));

  ioctl(fd, UI_DEV_CREATE);

  return fd;
}

// from examples at https://www.kernel.org/doc/html/v4.12/input/uinput.html
void emit(int fd, int type, int code, int val) {
  if (mode == MODE_HID)
    return;

  struct input_event e;
  e.type = type;
  e.code = code;
  e.value = val;
  gettimeofday(&e.time, NULL);
  write(fd, &e, sizeof(e));
}

int manage_input_mode(
    int changed, int pressed, int pressed_state, int released_state, int fd) {
  static int pressing = 0;
  static int configured = 0;

  if (pressing) {
    pressing++;

    if (pressed_state & B_A)
      rapid_a ^= 1;
    if (pressed_state & B_B)
      rapid_b ^= 1;
    if (pressed_state & B_X)
      rapid_x ^= 1;
    if (pressed_state & B_Y)
      rapid_y ^= 1;
    if (pressed_state & B_LEFT)
      emit(fd, EV_KEY, KEY_L, 1);
    if (released_state & B_LEFT)
      emit(fd, EV_KEY, KEY_L, 0);
    if (pressed_state & B_UP)
      emit(fd, EV_KEY, KEY_U, 1);
    if (released_state & B_UP)
      emit(fd, EV_KEY, KEY_U, 0);
    if (pressed_state & B_RIGHT)
      emit(fd, EV_KEY, KEY_R, 1);
    if (released_state & B_RIGHT)
      emit(fd, EV_KEY, KEY_R, 0);
    if (pressed_state & B_DOWN)
      emit(fd, EV_KEY, KEY_D, 1);
    if (released_state & B_DOWN)
      emit(fd, EV_KEY, KEY_D, 0);

    if (pressed_state &
        (B_LEFT | B_UP | B_RIGHT | B_DOWN | B_A | B_B | B_X | B_Y) ||
        released_state & (B_LEFT | B_UP | B_RIGHT | B_DOWN)) {
      configured = 1;
    }

    led_set(0, rapid_a);
    led_set(1, rapid_b);
    led_set(2, rapid_x);
    led_set(3, rapid_y);
  }

  if (!changed)
    return 0;

  if (pressed) {
    pressing = 1;
    return 0;
  }

  int mode_changed = 0;
  if (!configured && pressing > 1000) {
    mode = (mode + 1) % 3;
    mode_changed = 1;
  }

  pressing = 0;
  led_set(0, mode == 0);
  led_set(1, mode == 1);
  led_set(2, mode == 2);
  led_set(3, mode == 3);
  return mode_changed;
}

int main() {
  if (gpio_setup()) {
    perror("open /dev/mem");
    exit(EXIT_FAILURE);
  }

  int js_fd = js_setup();
  if (js_fd < 0)
    exit(EXIT_FAILURE);

  int hid_fd = open("/dev/hidg0", O_RDWR);
  signed char hid_report[4];
  hid_report[0] = 0x00;
  hid_report[1] = 0xf0;
  hid_report[2] = 0x00;
  hid_report[3] = 0x00;

  int old_raw_state = 0x3fffff;
  int old_cooked_state = 0x3fffff;

  for (;;) {
    usleep(2000);
    int new_state = gpio[0].dat;
    int changed_state = (old_raw_state ^ new_state) & B_ALL;
    old_raw_state = new_state;
    int pressed_state = changed_state & ~new_state;
    int released_state = changed_state & new_state;
    if ((rapid_count >> 4) & 1) {
      if (rapid_a)
        new_state |= B_A;
      if (rapid_b)
        new_state |= B_B;
      if (rapid_x)
        new_state |= B_X;
      if (rapid_y)
        new_state |= B_Y;
    }
    changed_state = (old_cooked_state ^ new_state) & B_ALL;
    old_cooked_state = new_state;
    int state = new_state;

    if (manage_input_mode(
        changed_state & B_XBOX, ~state & B_XBOX, pressed_state, released_state,
        js_fd)) {
      int new_fd = js_setup();
      if (new_fd >= 0) {
        close(js_fd);
        js_fd = new_fd;
      }
    }
    if (~state & B_XBOX)
      state |= (B_A | B_B | B_X | B_Y);

    rapid_count++;

    if (!changed_state)
      continue;

    if (changed_state & B_BACK ) {
      int code = (mode == MODE_KBD) ? KEY_ENTER : BTN_SELECT;
      emit(js_fd, EV_KEY, code, (state & B_BACK ) ? 0 : 1);
      hid_report[1] &= 0xfd;
      hid_report[1] |= (state & B_BACK ) ? 0 : 0x02;
    }
    if (changed_state & B_START) {
      int code = (mode == MODE_KBD) ? KEY_SPACE : BTN_START;
      emit(js_fd, EV_KEY, code, (state & B_START) ? 0 : 1);
      hid_report[1] &= 0xfe;
      hid_report[1] |= (state & B_START) ? 0 : 0x01;
    }
    if (changed_state & B_A    ) {
      int code = (mode == MODE_KBD) ? KEY_Z : BTN_A;
      emit(js_fd, EV_KEY, code, (state & B_A    ) ? 0 : 1);
      hid_report[0] &= 0xfe;
      hid_report[0] |= (state & B_A    ) ? 0 : 0x01;
    }
    if (changed_state & B_B    ) {
      int code = (mode == MODE_KBD) ? KEY_X : BTN_B;
      emit(js_fd, EV_KEY, code, (state & B_B    ) ? 0 : 1);
      hid_report[0] &= 0xfd;
      hid_report[0] |= (state & B_B    ) ? 0 : 0x02;
    }
    if (changed_state & B_X    ) {
      int code = (mode == MODE_KBD) ? KEY_A : BTN_X;
      emit(js_fd, EV_KEY, code, (state & B_X    ) ? 0 : 1);
      hid_report[0] &= 0xfb;
      hid_report[0] |= (state & B_X    ) ? 0 : 0x04;
    }
    if (changed_state & B_Y    ) {
      int code = (mode == MODE_KBD) ? KEY_S : BTN_Y;
      emit(js_fd, EV_KEY, code, (state & B_Y    ) ? 0 : 1);
      hid_report[0] &= 0xf7;
      hid_report[0] |= (state & B_Y    ) ? 0 : 0x08;
    }
    if (changed_state & B_RT   ) {
      int code = (mode == MODE_KBD) ? KEY_W : BTN_TR2;
      emit(js_fd, EV_KEY, code, (state & B_RT   ) ? 0 : 1);
      hid_report[0] &= 0xdf;
      hid_report[0] |= (state & B_RT   ) ? 0 : 0x20;
    }
    if (changed_state & B_LT   ) {
      int code = (mode == MODE_KBD) ? KEY_Q: BTN_TL2;
      emit(js_fd, EV_KEY, code, (state & B_LT   ) ? 0 : 1);
      hid_report[0] &= 0xef;
      hid_report[0] |= (state & B_LT   ) ? 0 : 0x10;
    }
    if (changed_state & B_RB   ) {
      int code = (mode == MODE_KBD) ? KEY_ESC: BTN_TR;
      emit(js_fd, EV_KEY, code, (state & B_RB   ) ? 0 : 1);
      hid_report[0] &= 0x7f;
      hid_report[0] |= (state & B_RB   ) ? 0 : 0x80;
    }
    if (changed_state & B_LB   ) {
      int code = (mode == MODE_KBD) ? KEY_ESC: BTN_TL;
      emit(js_fd, EV_KEY, code, (state & B_LB   ) ? 0 : 1);
      hid_report[0] &= 0xbf;
      hid_report[0] |= (state & B_LB   ) ? 0 : 0x40;
    }
    if (changed_state & B_XBOX ) {
      emit(js_fd, EV_KEY, BTN_MODE  , (state & B_XBOX ) ? 0 : 1);
      hid_report[1] &= 0xf3;
      hid_report[1] |= (state & B_XBOX ) ? 0 : 0x04;
    }

    if (changed_state & (B_LEFT | B_RIGHT)) {
      int val = !(state & B_LEFT) ? -32767 : !(state & B_RIGHT) ? 32767 : 0;
      if (mode == MODE_UINPUT)
        emit(js_fd, EV_ABS, ABS_X, val);
      val = !(state & B_LEFT) ? -127 : !(state & B_RIGHT) ? 127 : 0;
      hid_report[2] = val;
      if (mode == MODE_KBD) {
        emit(js_fd, EV_KEY, KEY_LEFT,  (state & B_LEFT ) ? 0 : 1);
        emit(js_fd, EV_KEY, KEY_RIGHT, (state & B_RIGHT) ? 0 : 1);
      }
    }
    if (changed_state & (B_UP | B_DOWN)) {
      int val = !(state & B_UP) ? -32767 : !(state & B_DOWN) ? 32767 : 0;
      if (mode == MODE_UINPUT)
        emit(js_fd, EV_ABS, ABS_Y, val);
      val = !(state & B_UP) ? -127 : !(state & B_DOWN) ? 127 : 0;
      hid_report[3] = val;
      if (mode == MODE_KBD) {
        emit(js_fd, EV_KEY, KEY_UP,    (state & B_UP   ) ? 0 : 1);
        emit(js_fd, EV_KEY, KEY_DOWN,  (state & B_DOWN ) ? 0 : 1);
      }
    }
    hid_report[1] &= 0x0f;
    if (hid_report[2] == -127) {
      if (hid_report[3] == -127)
        hid_report[1] |= 0x70;
      else if (hid_report[3] == 0)
        hid_report[1] |= 0x60;
      else
        hid_report[1] |= 0x50;
    } else if (hid_report[2] == 0) {
      if (hid_report[3] == -127)
        hid_report[1] |= 0x00;
      else if (hid_report[3] == 0)
        hid_report[1] |= 0xf0;
      else
        hid_report[1] |= 0x40;
    } else {
      if (hid_report[3] == -127)
        hid_report[1] |= 0x10;
      else if (hid_report[3] == 0)
        hid_report[1] |= 0x20;
      else
        hid_report[1] |= 0x30;
    }

    emit(js_fd, EV_SYN, SYN_REPORT, 0);

    if (mode == MODE_HID && hid_fd >= 0)
      write(hid_fd, hid_report, 4);

  }
  return 0;
}
