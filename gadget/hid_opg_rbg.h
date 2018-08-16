static char opg_driver_name[] =
    "OPiPad Gadget HID Driver for retro-bit GENERATIONS";

#define OPG_USB_VERSION cpu_to_le16(0x200)
#define OPG_DEVICE_CLASS USB_CLASS_PER_INTERFACE
#define OPG_DEVICE_SUB_CLASS 0
#define OPG_DEVICE_PROTOCOL 0

#define OPG_VENDOR_ID 0x6666
#define OPG_PRODUCT_ID 0x0880

static char opg_hid_report[] = {
  0x05, 0x01,  // usage page (desktop)
  0x09, 0x04,  // usage (sport control)
  0xa1, 0x01,  // collection (application)
  0x75, 0x04,  // report size (4)
  0x95, 0x02,  // report count (2)
  0x15, 0x00,  // logical minimum (0)
  0x25, 0x0f,  // logical maximum (15)
  0x35, 0x00,  // physical minimum (0)
  0x45, 0xff,  // physical maximum (255)
  0x09, 0x30,  // usage (x)
  0x09, 0x31,  // usage (y)
  0x81, 0x02,  // input (variable)
  0x75, 0x01,  // report size (1)
  0x95, 0x06,  // report count (6)
  0x25, 0x01,  // logical maximum (1)
  0x45, 0x01,  // physical maximum (1)
  0x05, 0x09,  // usage page (button)
  0x19, 0x01,  // usage minimum (1)
  0x29, 0x06,  // usage maximum (6)
  0x81, 0x02,  // input (variable)
  0x95, 0x02,  // report count (2)
  0x19, 0x09,  // usage minimum (9)
  0x29, 0x0a,  // usage maximum (10)
  0x81, 0x02,  // input (variable)
  0xc0,  // end collection
};

struct opg_config_descriptor {
  struct usb_config_descriptor config;
  struct usb_interface_descriptor interface;
  struct usb_hid_descriptor hid;
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
    .bNumEndpoints       = 1,
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
  .ep_in = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN | 4,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 8,
    .bInterval           = 5,
  },
};

static __u8 opg_report[] = {
  //XY  start mode C Z X A B Y
  0x88, 0x00,
};

static const char* opg_get_string(int idx) {
  switch (idx) {
    case IDX_MANUFACTURER:
      return "TOYOSHIMA-HOUSE";
    case IDX_PRODUCT:
      return "OPiPad retro-bit GENERATIONS Adaptor";
    case IDX_SERIAL:
      return "0";
    default:
      break;
  }
  return NULL;
}

static void opg_update_report(void) {
  const struct gpio_hid_state* state = gpio_get_state();
  opg_report[0] =
      (!state->left ? 0 : !state->right ? 0x0f: 0x08) |
      (!state->up ? 0 : !state->down ? 0xf0: 0x80);
  opg_report[1] =
      (state->x ? 0 : 0x01) |
      (state->lt ? 0 : 0x02) |
      (state->a ? 0 : 0x04) |
      (state->b ? 0 : 0x08) |
      (state->y ? 0 : 0x10) |
      (state->rt ? 0 : 0x20) |
      (state->back ? 0 : 0x40) |
      (state->start ? 0 : 0x80);
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
        opg_update_report();
        memcpy(data->ep0_request->buf, opg_report, sizeof(opg_report));
        return sizeof(opg_report);
      default:
        printk("%s: report type/id: %04x\n", opg_driver_name, r->wValue);
        break;
    }
  }
  return -EOPNOTSUPP;
}
