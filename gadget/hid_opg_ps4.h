static char opg_driver_name[] = "OPiPad Gadget HID Driver for PS4";

#define OPG_VENDOR_ID 0x6666
#define OPG_PRODUCT_ID 0x0884

static char opg_hid_report[] = {
  0x05, 0x01,  // usage page (desktop)
  0x09, 0x05,  // usage (gamepad)
  0xa1, 0x01,  // collection (application)
  0x85, 0x01,  // report id (1)
  0x09, 0x30,  // usage (x)
  0x09, 0x31,  // usage (y)
  0x09, 0x32,  // usage (z)
  0x09, 0x35,  // usage (rz)
  0x15, 0x00,  // logical minimum (0)
  0x26, 0xff, 0x00,  // logical maximum (255)
  0x75, 0x08,  // report size (8)
  0x95, 0x04,  // report count (4)
  0x81, 0x02,  // input (variable)
  0x09, 0x39,  // usage (hat switch)
  0x15, 0x00,  // logical minimum (0)
  0x25, 0x07,  // logical maximum (7)
  0x35, 0x00,  // physical minimum (0)
  0x46, 0x3b, 0x01,  // physical maximum (315)
  0x65, 0x14,  // unit (degrees)
  0x75, 0x04,  // report size (4)
  0x95, 0x01,  // report count (1)
  0x81, 0x42,  // input (variable, null state)
  0x65, 0x00,  // unit
  0x05, 0x09,  // usage page (button)
  0x19, 0x01,  // usage minimum (1)
  0x29, 0x0e,  // usage maximum (14)
  0x15, 0x00,  // logical minimum (0)
  0x25, 0x01,  // logical maximum (1)
  0x75, 0x01,  // report size (1)
  0x95, 0x0e,  // report count (14)
  0x81, 0x02,  // input (variable)
  0x06, 0x00, 0xff,  // usage page (ff00h)
  0x09, 0x20,  // usage (20h)
  0x75, 0x06,  // report size (6)
  0x95, 0x01,  // report count (1)
  0x81, 0x02,  // input (variable)
  0x05, 0x01,  // usage page (desktop)
  0x09, 0x33,  // usage (rx)
  0x09, 0x34,  // usage (ry)
  0x15, 0x00,  // logical minimum (0)
  0x26, 0xff, 0x00,  // logical maximum (255)
  0x75, 0x08,  // report size (8)
  0x95, 0x02,  // report count (2)
  0x81, 0x02,  // input (variable)
  0x06, 0x00, 0xff,  // usage page (ff00h)
  0x09, 0x21,  // usage (21h)
  0x95, 0x36,  // report count (54)
  0x81, 0x02,  // input (variable)
  0x85, 0x05,  // report id (5)
  0x09, 0x22,  // usage (22h)
  0x95, 0x1f,  // report count (31)
  0x91, 0x02,  // output (variable)
  0x85, 0x03,  // report id (3)
  0x0a, 0x21, 0x27,  // usage (2721h)
  0x95, 0x2f,  // report count (47)
  0xb1, 0x02,  // feature (variable)
  0xc0,  // end collection
  0x06, 0xf0, 0xff,  // usage page (fff0h)
  0x09, 0x40,  // usage (40h)
  0xa1, 0x01,  // collection (application)
  0x85, 0xf0,  // report id (240)
  0x09, 0x47,  // usage (47h)
  0x95, 0x3f,  // report count (63)
  0xb1, 0x02,  // feature (variable)
  0x85, 0xf1,  // report id (241)
  0x09, 0x48,  // usage (48h)
  0x95, 0x3f,  // report count (63)
  0xb1, 0x02,  // feature (variable)
  0x85, 0xf2,  // report id (242)
  0x09, 0x49,  // usage (49h)
  0x95, 0x0f,  // report count (15)
  0xb1, 0x02,  // feature (variable)
  0x85, 0xf3,  // report id (243)
  0x0a, 0x01, 0x47,  // usage (4701h)
  0x95, 0x07,  // report count (7)
  0xb1, 0x02,  // feature (variable)
  0xc0,  // end collection
};

struct opg_config_descriptor {
  struct usb_config_descriptor config;
  struct usb_interface_descriptor interface;
  struct usb_hid_descriptor hid;
  struct usb_short_endpoint_descriptor ep_in;
  struct usb_short_endpoint_descriptor ep_out;
} __attribute__ ((packed)) opg_config_desc = {
  .config = {
    .bLength             = USB_DT_CONFIG_SIZE,
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = cpu_to_le16(sizeof(struct opg_config_descriptor)),
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = IDX_NULL,
    .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower           = 250,  // x 2mA
  },
  .interface = {
    .bLength             = USB_DT_INTERFACE_SIZE,
    .bDescriptorType     = USB_DT_INTERFACE,
    .bInterfaceNumber    = 0,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 2,
    .bInterfaceClass     = USB_CLASS_HID,
    .bInterfaceSubClass  = 0x00,
    .bInterfaceProtocol  = 0x00,
    .iInterface          = IDX_NULL,
  },
  .hid = {
    .bLength             = sizeof(struct usb_hid_descriptor),
    .bDescriptorType     = USB_DT_HID,
    .bcdHID              = cpu_to_le16(0x0111),
    .bCountryCode        = 0,
    .bNumReports         = 1,
    .bReportType         = USB_DT_HID_REPORT,
    .wReportLength       = cpu_to_le16(sizeof(opg_hid_report)),
  },
  .ep_in = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN | 4,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 5,
  },
  .ep_out = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_OUT | 3,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 5,
  },
};

static __u8 opg_report[] = {
  // report ID
  0x01,

  //  X     Y     Z    Rz
  0x80, 0x80, 0x80, 0x80,
  // 4 Buttons | 4-bits Hat switch
  0x00 | 0x08,
  // 10 Buttons, 6 vendor specific unknown
  0x00, 0x00,
  // Rx    Ry
  0x00, 0x00,

  // vendor specific unknown
  0x99, 0x50, 0xff, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xe7, 0xff, 0x6e, 0x20, 0xd9,
  0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80,
  0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00
};

static const char report0303[] =
{
  0x03, 0x21, 0x27, 0x04, 0x41, 0x00, 0x2c, 0x56,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char report03f3[] =
{
  0xf3, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00
};

static const char* opg_get_string(int idx) {
  switch (idx) {
    case IDX_MANUFACTURER:
      return "TOYOSHIMA-HOUSE";
    case IDX_PRODUCT:
      return "OPiPad PS4 Adaptor";
    default:
      break;
  }
  return NULL;
}

static void opg_update_report(void) {
  const struct gpio_hid_state* state = gpio_get_state();
  opg_report[1] = !state->left ? 0 : !state->right ? 0xff: 0x80;
  opg_report[2] = !state->up ? 0 : !state->down ? 0xff: 0x80;
  opg_report[5] = 0x08 |
    (!state->y ? 0x80 : 0) |
    (!state->b ? 0x40 : 0) |
    (!state->a ? 0x20 : 0) |
    (!state->x ? 0x10 : 0);
  opg_report[6] =
    (!state->start ? 0x20 : 0) |
    (!state->back  ? 0x10 : 0) |
    (!state->rt    ? 0x08 : 0) |
    (!state->lt    ? 0x04 : 0) |
    (!state->rb    ? 0x02 : 0) |
    (!state->lb    ? 0x01 : 0);
  opg_report[7] = (opg_report[7] & 0xfc) | (!state->meta ? 0x01: 0);
  opg_report[7] += 4;  // count-up
}

static int opg_setup(
    struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  int type = r->bRequestType & USB_TYPE_MASK;
  if (type == USB_TYPE_CLASS && r->bRequest == HID_REQ_GET_REPORT) {
    switch(le16_to_cpu(r->wValue)) {
      case 0x0303:
        memcpy(data->ep0_request->buf, report0303, sizeof(report0303));
        return sizeof(report0303);
      case 0x03f2:
        opg_update_report();
        memcpy(data->ep0_request->buf, opg_report, sizeof(opg_report));
        return sizeof(opg_report);
      case 0x03f3:
        memcpy(data->ep0_request->buf, report03f3, sizeof(report03f3));
        return sizeof(report03f3);
      default:
        printk("%s: report page: %04x\n", opg_driver_name, r->wValue);
        break;
    }
  }
  return -EOPNOTSUPP;
}
