```
==== Orange Pi as a Game Pad =========================
_______     _____________    ________       _________
__  __ \    ___  __ \__(_)   ___  __ \_____ ______  /
_  / / /    __  /_/ /_  /    __  /_/ /  __ `/  __  / 
/ /_/ /     _  ____/_  /     _  ____// /_/ // /_/ /  
\____/      /_/     /_/      /_/     \__,_/ \__,_/   
```

# What?
This is a project to use Orange Pi One as a gamepad.
You can connect your own arcade controller to the Pi's GPIO,
and allow Linux to recognize it as a joystick (e.g., /dev/js0).

Also, the project includes a USB Gadget API driver
to expose the controller to PC as a USB gamepad.

You may be able to run it on other Orange Pi series
or Raspberry Pi Zero (W) with small code modifications.

# GPIO pin assign (an example)
This is an exmplae pin assign for Hori Fighting Stick EX (Xbox 360).
You can modify source code to customize it.

|     |   |   |      |
|:---:|:-:|:-:|:----:|
|3.3V |1  |2  |5V    |
|BACK |3  |4  |5V    |
|START|5  |6  |GND   |
|UP   |7  |8  |RB    |
|GND  |9  |10 |LB    |
|DOWN |11 |12 |(PD14)|
|LEFT |13 |14 |GND   |
|RIGHT|15 |16 |(PC4) |
|3.3V |17 |18 |(PC7) |
|(PC0)|19 |20 |GND   |
|(PC1)|21 |22 |Xbox  |
|(PC2)|23 |24 |(PC3) |
|GND  |25 |26 |(PA21)|
|A    |27 |28 |(PA18)|
|B    |29 |30 |GND   |
|X    |31 |32 |(PG8) |
|Y    |33 |34 |GND   |
|RT   |35 |36 |(PG9) |
|LT   |37 |38 |(PG6) |
|GND  |39 |40 |(PG7) |

# Hardware
```
Button/Stick circuit

   ___ ... GPIO pin (w/ internal pull-up)
  |
 \
  \
  |
__|__
///// GND
```

# Install
On Armbian, or Retrorange Pi 3,
```
% cd /opt
% git clone https://github.com/toyoshim/opipad.git
% cd opipad
% make
% cat rc.local.example >> /etc/rc.local
```

On Armbian for Orange Pi Zero, you will need to disable other USB Gadget modules, such as g_serial.
You can disable it by commenting it out in /etc/modules.

If the system does not have /lib/modules/3.4.113-sub8i/build (maybe happen on Retrorange Pi 3),
you need to create a symlink from /usr/src/linux-headers-3.4.113-sun8i to compile drivers.

# How to use
Once the applications are built, and the hardware is set up, it just works after the next reboot.
Pressing Xbox button will switch the running mode. The first mode is local controller mode,
and the second one is USB gamepad mode.
