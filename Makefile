# Copyright 2017 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

obj-m := hid_opp.o

all:
	make uin_oppd
	make uin_oppj
	make input2gpio
	make modules

modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	rm uin_oppd uin_oppj input2gpio
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
