// Copyright 2017 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/g_hid.h>

MODULE_LICENSE("Dual BSD/GPL");

static struct hidg_func_descriptor hid_data = {
	.subclass		= 0,
	.protocol		= 0,
	.report_length		= 4,
	.report_desc_length	= 69,
	.report_desc		= {
		0x05, 0x01,		// USAGE_PAGE (Generic Desktop)
		0x09, 0x05,		// USAGE (Gamepad)

		0xa1, 0x01,		// COLLECTION (Application)

		0x05, 0x09,		// USAGE_PAGE (Button)
		0x15, 0x00,		// LOGICAL MINIMUM (0)
		0x25, 0x01,		// LOGICAL MAXIMUM (1)
		0x19, 0x01,		// USAGE MINIMUM (1)
		0x29, 0x0b,		// USAGE MAXIMUM (11)
		0x75, 0x01,		// REPORT SIZE (1)
		0x95, 0x0b,		// REPORT COUNT (11)
		0x81, 0x02,		// INPUT (DATA, VARIABLE, ABS)

		0x95, 0x01,		// REPORT COUNT (1)
		0x81, 0x01,		// INPUT (CONSTANT, ARRAY, ABS)

		0x05, 0x01,		// USAGE_PAGE (Generic Desktop)
		0x09, 0x39,		// USAGE (Hat switch)
		0x25, 0x07,		// LOGICAL MAXIMUM (7)
		0x35, 0x00,		// PHYSICAL MINIMUM (0)
		0x46, 0x3b, 0x01,	// PHYSICAL MAXIMUM (315)
		0x75, 0x04,		// REPORT SIZE (4)
		0x95, 0x01,		// REPORT COUNT (1)
		0x65, 0x14,		// UNIT (Degrees)
		0x81, 0x42,		// INPUT (DATA, VARIABLE, ABS, NULL)

		0x09, 0x01,		// USAGE (Pointer)
		0x15, 0x81,		// LOGICAL MINIMUM (-127)
		0x25, 0x7f,		// LOGICAL MAXIMUM (127)
		0x35, 0x81,		// PHYSICAL MINIMUM (-127)
		0x45, 0x7f,		// PHYSICAL MAXIMUM (127)
		0xa1, 0x00,		// COLLECTION (Physical)
		0x09, 0x30,		// USAGE (X)
		0x09, 0x31,		// USAGE (Y)
		0x75, 0x08,		// REPORT SIZE (8)
		0x95, 0x02,		// REPORT COUNT (2)
		0x81, 0x02,		// INPUT (DATA, VARIABLE, ABS)
		0xc0,			// END_COLLECTION

		0xc0,			// END_COLLECTION
	}
};

static void hid_release(struct device *dev) {}

static struct platform_device hid = {
	.name			= "hidg",
	.id			= 0,
	.num_resources		= 0,
	.resource		= 0,
	.dev.platform_data	= &hid_data,
        .dev.release            = &hid_release,
};

static int hid_init(void) {
	return platform_device_register(&hid);
}

static void hid_exit(void) {
	platform_device_unregister(&hid);
}

module_init(hid_init);
module_exit(hid_exit);
