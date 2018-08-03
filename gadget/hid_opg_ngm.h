static char opg_driver_name[] = "OPiPad Gadget HID Driver for NEOGEO mini";

#define OPG_USB_VERSION cpu_to_le16(0x110)
#define OPG_DEVICE_CLASS USB_CLASS_PER_INTERFACE
#define OPG_DEVICE_SUB_CLASS 0
#define OPG_DEVICE_PROTOCOL 0

#define OPG_VENDOR_ID 0x20bc
#define OPG_PRODUCT_ID 0x5500

#define NO_IDX_PRODUCT 1
#define NO_IDX_SERIAL 1

static char opg_hid_report[] = {
  0x05, 0x01,  // usage page (desktop)
  0x09, 0x05,  // usage (gamepad)
  0xa1, 0x01,  // collection (application)
  0x15, 0x00,  // logical minimum (0)
  0x25, 0x01,  // logical maximum (1)
  0x35, 0x00,  // physical minimum (0)
  0x45, 0x01,  // physical minimum (1)
  0x75, 0x01,  // report size (1)
  0x95, 0x0f,  // report count (15)
  0x05, 0x09,  // usage page (button)
  0x19, 0x01,  // usage minimum (1)
  0x29, 0x0f,  // usage maximum (15)
  0x81, 0x02,  // input (variable)
  0x95, 0x01,  // report count (1)
  0x81, 0x01,  // input (constant)
  0x05, 0x01,  // usage page (desktop)
  0x25, 0x07,  // logical maximum (7)
  0x46, 0x3b, 0x01,  // physical maximum (315)
  0x75, 0x04,  // report size (4)
  0x95, 0x01,  // report count (1)
  0x65, 0x14,  // unit (degrees)
  0x09, 0x39,  // usage (hat switch)
  0x81, 0x42,  // input (variable, null state)
  0x65, 0x00,  // unit
  0x95, 0x01,  // report count (1)
  0x81, 0x01,  // input (constant)
  0x26, 0xff, 0x00,  // logical maximum (255)
  0x46, 0xff, 0x00,  // physical maximum (255)
  0x09, 0x30,  // usage (x)
  0x09, 0x31,  // usage (y)
  0x09, 0x32,  // usage (z)
  0x09, 0x35,  // usage (rz)
  0x75, 0x08,  // report size (8)
  0x95, 0x04,  // report count (4)
  0x81, 0x02,  // input (variable)
  0x05, 0x02,  // usage page (simulation)
  0x15, 0x00,  // logical minimum (0)
  0x26, 0xff, 0x00,  // logical maximum (255)
  0x09, 0xc5,  // usage (c5h)
  0x09, 0xc4,  // usage (c4h)
  0x09, 0x02,  // usage (02h)
  0x75, 0x08,  // report size (8)
  0x81, 0x02,  // input (array)
  0xc0,  // end collection
};

struct opg_config_descriptor {
  struct usb_config_descriptor config;
  struct usb_interface_descriptor interface;
  struct usb_hid_descriptor hid;
  struct usb_short_endpoint_descriptor ep_out;
  struct usb_short_endpoint_descriptor ep_in;
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
    .bcdHID              = cpu_to_le16(0x0110),
    .bCountryCode        = 0,
    .bNumReports         = 1,
    .bReportType         = USB_DT_HID_REPORT,
    .wReportLength       = cpu_to_le16(sizeof(opg_hid_report)),
  },
  .ep_out = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_OUT | 3,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 32,
    .bInterval           = 10,
  },
  .ep_in = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN | 4,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 32,
    .bInterval           = 10,
  },
};

static __u8 opg_report[] = {
  // _ _ _ D | C _ B A | _ _ _ _ | START SELECT _ _
  0x00, 0x00,

  // Hat
  // 7 0 1
  // 6 f 2
  // 5 4 3
  0x0f,
};

static const char* opg_get_string(int idx) {
  switch (idx) {
    case IDX_MANUFACTURER:
      return "JJ";
    default:
      break;
  }
  return NULL;
}

static void opg_update_report(void) {
  const struct gpio_hid_state* state = gpio_get_state();
  opg_report[0] =
    (!state->a ? 0x01 : 0) |
    (!state->b ? 0x02 : 0) |
    (!state->x ? 0x08 : 0) |
    (!state->y ? 0x10 : 0);
  opg_report[1] =
    (!state->back ? 0x04 : 0) | (!state->start ? 0x08 : 0);

  opg_report[2] =
    !state->up ? (!state->left ? 7 : !state->right ? 1 : 0) :
    !state->down ? (!state->left ? 5 : !state->right ? 3: 4) :
    (!state->left ? 6 : !state->right ? 2 : 15);
}

static int opg_setup(
    struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  int type = r->bRequestType & USB_TYPE_MASK;
  printk("%s: bRequestType: %02x, bRequest: %02x, wValue: %04x, "
      "wIndex: %04x, wLendth: %04x\n",
      opg_driver_name, r->bRequestType, r->bRequest, r->wValue, r->wIndex,
      r->wLength);
  if (type == USB_TYPE_CLASS && r->bRequest == HID_REQ_GET_REPORT) {
    switch(le16_to_cpu(r->wValue)) {
      case 0x0100:
        memcpy(data->ep0_request->buf, opg_report, sizeof(opg_report));
        return sizeof(opg_report);
      default:
        printk("%s: report type/id: %04x\n", opg_driver_name, r->wValue);
        break;
    }
  }
  return -EOPNOTSUPP;
}
