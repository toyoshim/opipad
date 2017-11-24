#include <linux/gpio.h>

struct gpio_hid_state {
  unsigned back:1;
  unsigned start:1;
  unsigned up:1;
  unsigned down:1;
  unsigned left:1;
  unsigned right:1;
  unsigned a:1;
  unsigned b:1;
  unsigned x:1;
  unsigned y:1;
  unsigned rt:1;
  unsigned lt:1;
  unsigned rb:1;
  unsigned lb:1;
  unsigned meta:1;
};

static const struct gpio_hid_state* gpio_get_state(void) {
  static struct gpio_hid_state state;
  state.back  = __gpio_get_value(12);
  state.start = __gpio_get_value(11);
  state.up    = __gpio_get_value( 6);
  state.down  = __gpio_get_value( 1);
  state.left  = __gpio_get_value( 0);
  state.right = __gpio_get_value( 3);
  state.a     = __gpio_get_value(19);
  state.b     = __gpio_get_value( 7);
  state.x     = __gpio_get_value( 8);
  state.y     = __gpio_get_value( 9);
  state.rt    = __gpio_get_value(10);
  state.lt    = __gpio_get_value(20);
  state.rb    = __gpio_get_value(13);
  state.lb    = __gpio_get_value(14);
  state.meta  = __gpio_get_value( 2);
  return &state;
}

