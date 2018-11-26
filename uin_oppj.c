// Copyright 2018 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
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

#define OUT0 (1u << 12)
#define OUT1 (1u << 11)
#define OUT2 (1u << 6)

#define P2D4 (1u << 1)
#define P2D2 (1u << 0)
#define P2D3 (1u << 3)
#define P2D1 (1u << 15)
#define SEL (1u << 14)

#define MJ_A (1u << 0)
#define MJ_B (1u << 1)
#define MJ_C (1u << 2)
#define MJ_D (1u << 3)
#define MJ_E (1u << 4)
#define MJ_F (1u << 5)
#define MJ_G (1u << 6)
#define MJ_H (1u << 7)
#define MJ_I (1u << 8)
#define MJ_J (1u << 9)
#define MJ_K (1u << 10)
#define MJ_L (1u << 11)
#define MJ_M (1u << 12)
#define MJ_N (1u << 13)
#define MJ_KAN (1u << 14)
#define MJ_PON (1u << 15)
#define MJ_CHI (1u << 16)
#define MJ_REACH (1u << 17)
#define MJ_RON (1u << 18)
#define MJ_SEL (1u << 19)

volatile struct pio_cfg {
  volatile uint32_t cfg[4];
  volatile uint32_t dat;
  volatile uint32_t drv[2];
  volatile uint32_t pul[2];
}* gpio;

int gpio_setup() {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0)
    return fd;

  int prot = PROT_READ | PROT_WRITE;
  size_t size = sysconf(_SC_PAGE_SIZE);
  char *gpio_base = mmap(NULL, size, prot, MAP_SHARED, fd, GPIO_PA_BASE);
  gpio = (struct pio_cfg*)&gpio_base[GPIO_PA_OFFSET];

  gpio[0].cfg[0] &= ~0x0f00f0ff;  // PA07-PA00: input [6,3,1,0]
  gpio[0].cfg[0] |=  0x01000000;  // PA07-PA00: output [6]
  gpio[0].cfg[1] &= ~0xff0ff000;  // PA15-PA08: input [15,14,12,11]
  gpio[0].cfg[1] |=  0x00011000;  // PA15-PA08: output [12,11]
  gpio[0].pul[0] &= ~0xf00000cf;  // PA15-PA00: no-pull-up/down [15,14,3,1,0]
  gpio[0].pul[0] |=  0x20000000;  // PA15-PA00: pull-down [14]

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

  ioctl(fd, UI_SET_KEYBIT, KEY_A);
  ioctl(fd, UI_SET_KEYBIT, KEY_B);
  ioctl(fd, UI_SET_KEYBIT, KEY_C);
  ioctl(fd, UI_SET_KEYBIT, KEY_D);
  ioctl(fd, UI_SET_KEYBIT, KEY_E);
  ioctl(fd, UI_SET_KEYBIT, KEY_F);
  ioctl(fd, UI_SET_KEYBIT, KEY_G);
  ioctl(fd, UI_SET_KEYBIT, KEY_H);
  ioctl(fd, UI_SET_KEYBIT, KEY_I);
  ioctl(fd, UI_SET_KEYBIT, KEY_J);
  ioctl(fd, UI_SET_KEYBIT, KEY_K);
  ioctl(fd, UI_SET_KEYBIT, KEY_L);
  ioctl(fd, UI_SET_KEYBIT, KEY_M);
  ioctl(fd, UI_SET_KEYBIT, KEY_N);
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL);   // Kan
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTALT);    // Pon
  ioctl(fd, UI_SET_KEYBIT, KEY_SPACE);      // Chi
  ioctl(fd, UI_SET_KEYBIT, KEY_LEFTSHIFT);  // Reach
  ioctl(fd, UI_SET_KEYBIT, KEY_Z);          // Ron

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
  snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "O-Pi Mahjang");
  int i;
  for (i = 0; i < ABS_CNT; ++i) {
    setup.absmax[i] =  32767;
    setup.absmin[i] = -32767;
  }
  write(fd, &setup, sizeof(setup));

  ioctl(fd, UI_DEV_CREATE);

  return fd;
}

void mj_reset() {
  gpio[0].dat &= ~OUT2;
  usleep(1000);
  gpio[0].dat |= OUT2;
  usleep(1000);
}

void mj_start_up() {
  gpio[0].dat |= OUT0;
}

void mj_start_down() {
  gpio[0].dat &= ~OUT0;
}

void mj_clock_up() {
  gpio[0].dat |= OUT1;
}

void mj_clock_down() {
  gpio[0].dat &= ~OUT1;
}

uint32_t mj_update() {
  const int delay = 5;
  uint32_t data[20];
  mj_start_up();
  int offset = 0;
  while (offset < 20) {
    mj_clock_down();
    usleep(delay);
    mj_start_down();
    data[offset++] = gpio[0].dat;
    usleep(delay);
    mj_clock_up();
    usleep(delay);
    data[offset++] = gpio[0].dat;
    usleep(delay);
  }
  uint32_t result = 0;
  if (data[ 0] & P2D3) result |= MJ_A;
  if (data[ 0] & P2D2) result |= MJ_B;
  if (data[16] & P2D2) result |= MJ_C;
  if (data[ 6] & P2D2) result |= MJ_D;
  if (data[13] & P2D4) result |= MJ_E;
  if (data[12] & P2D3) result |= MJ_F;
  if (data[ 6] & P2D3) result |= MJ_G;
  if (data[12] & P2D4) result |= MJ_H;
  if (data[16] & P2D3) result |= MJ_I;
  if (data[ 7] & P2D2) result |= MJ_J;
  if (data[15] & P2D2) result |= MJ_K;
  if (data[10] & P2D1) result |= MJ_L;
  if (data[17] & P2D2) result |= MJ_M;
  if (data[ 2] & P2D3) result |= MJ_N;
  if (data[ 0] & P2D4) result |= MJ_KAN;
  if (data[11] & P2D2) result |= MJ_PON;
  if (data[17] & P2D4) result |= MJ_CHI;
  if (data[ 2] & P2D2) result |= MJ_REACH;
  if (data[ 1] & P2D2) result |= MJ_RON;
  if (data[ 0] & SEL ) result |= MJ_SEL;
  return result;
}

// from examples at https://www.kernel.org/doc/html/v4.12/input/uinput.html
void emit(int fd, int type, int code, int val) {
  struct input_event e;
  e.type = type;
  e.code = code;
  e.value = val;
  gettimeofday(&e.time, NULL);
  write(fd, &e, sizeof(e));
}

void emits(int fd, uint32_t shift, uint32_t changed_state, uint32_t state) {
  if (changed_state & MJ_A)
    emit(fd, EV_KEY, shift ? BTN_A : KEY_A, (state & MJ_A) ? 1 : 0);
  if (changed_state & MJ_B)
    emit(fd, EV_KEY, shift ? BTN_B : KEY_B, (state & MJ_B) ? 1 : 0);
  if (changed_state & MJ_C)
    emit(fd, EV_KEY, shift ? BTN_X : KEY_C, (state & MJ_C) ? 1 : 0);
  if (changed_state & MJ_D)
    emit(fd, EV_KEY, shift ? BTN_Y : KEY_D, (state & MJ_D) ? 1 : 0);
  if (changed_state & MJ_E)
    emit(fd, EV_KEY, shift ? BTN_TL : KEY_E, (state & MJ_E) ? 1 : 0);
  if (changed_state & MJ_F)
    emit(fd, EV_KEY, shift ? BTN_TR : KEY_F, (state & MJ_F) ? 1 : 0);
  if (changed_state & MJ_G)
    emit(fd, EV_KEY, shift ? BTN_TL2 : KEY_G, (state & MJ_G) ? 1 : 0);
  if (changed_state & MJ_H)
    emit(fd, EV_KEY, shift ? BTN_TR2 : KEY_H, (state & MJ_H) ? 1 : 0);
  if (changed_state & MJ_I)
    emit(fd, EV_KEY, shift ? BTN_THUMBL : KEY_I, (state & MJ_I) ? 1 : 0);

  if (changed_state & MJ_N)
    emit(fd, EV_KEY, shift ? BTN_START : KEY_N, (state & MJ_N) ? 1 : 0);
  if (changed_state & MJ_RON)
    emit(fd, EV_KEY, shift ? BTN_SELECT : KEY_Z, (state & MJ_RON) ? 1 : 0);

  if (changed_state & MJ_J) {
    if (shift)
      emit(fd, EV_ABS, ABS_X, (state & MJ_J) ? -32767 : 0);
    else
      emit(fd, EV_KEY, KEY_J, (state & MJ_J) ? 1 : 0);
  }
  if (changed_state & MJ_K) {
    if (shift)
      emit(fd, EV_ABS, ABS_Y, (state & MJ_K) ? 32767 : 0);
    else
      emit(fd, EV_KEY, KEY_K, (state & MJ_K) ? 1 : 0);
  }
  if (changed_state & MJ_L) {
    if (shift)
      emit(fd, EV_ABS, ABS_Y, (state & MJ_L) ? -32767 : 0);
    else
      emit(fd, EV_KEY, KEY_L, (state & MJ_L) ? 1 : 0);
  }
  if (changed_state & MJ_M) {
    if (shift)
      emit(fd, EV_ABS, ABS_X, (state & MJ_M) ? 32767 : 0);
    else
      emit(fd, EV_KEY, KEY_M, (state & MJ_M) ? 1 : 0);
  }
  if (changed_state & MJ_PON) {
    if (shift)
      emit(fd, EV_ABS, ABS_HAT0X, (state & MJ_PON) ? -32767 : 0);
    else
      emit(fd, EV_KEY, KEY_LEFTALT, (state & MJ_PON) ? 1 : 0);
  }
  if (changed_state & MJ_CHI) {
    if (shift)
      emit(fd, EV_ABS, ABS_HAT0Y, (state & MJ_CHI) ? 32767 : 0);
    else
      emit(fd, EV_KEY, KEY_SPACE, (state & MJ_CHI) ? 1 : 0);
  }
  if (changed_state & MJ_KAN) {
    if (shift)
      emit(fd, EV_ABS, ABS_HAT0Y, (state & MJ_KAN) ? -32767 : 0);
    else
      emit(fd, EV_KEY, KEY_LEFTCTRL, (state & MJ_KAN) ? 1 : 0);
  }
  if (changed_state & MJ_REACH) {
    if (shift)
      emit(fd, EV_ABS, ABS_HAT0X, (state & MJ_REACH) ? 32767 : 0);
    else
      emit(fd, EV_KEY, KEY_LEFTSHIFT, (state & MJ_REACH) ? 1 : 0);
  }
}

int main() {
  if (gpio_setup()) {
    perror("open /dev/mem");
    exit(EXIT_FAILURE);
  }

  int js_fd = js_setup();
  if (js_fd < 0)
    exit(EXIT_FAILURE);

  mj_reset();

  for (;;) {
    usleep(2000);

    static uint32_t state = 0u;
    static uint32_t shift = 0u;
    uint32_t new_state = mj_update();
    uint32_t changed_state = state ^ new_state;

    if (changed_state & MJ_SEL) {
      if (state) {
        emits(js_fd, shift, state, 0u);
        emit(js_fd, EV_SYN, SYN_REPORT, 0);
      }
      state = 0u;
      changed_state = new_state;
    }
    shift = new_state & MJ_SEL;
    state = new_state;

    if (!changed_state)
      continue;

    emits(js_fd, shift, changed_state, state);
    emit(js_fd, EV_SYN, SYN_REPORT, 0);
  }

  return 0;
}
