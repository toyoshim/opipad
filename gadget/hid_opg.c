#include <linux/hid.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

enum {
  IDX_NULL,
  IDX_MANUFACTURER,
  IDX_PRODUCT,
  IDX_SERIAL,

  IDX_USER,
};

#define USB_DT_HID 0x21
#define USB_DT_HID_REPORT 0x22

struct usb_short_endpoint_descriptor {
  __u8   bLength;
  __u8   bDescriptorType;
  __u8   bEndpointAddress;
  __u8   bmAttributes;
  __le16 wMaxPacketSize;
  __u8   bInterval;
} __attribute__ ((packed));

struct usb_hid_descriptor {
  __u8   bLength;
  __u8   bDescriptorType;
  __le16 bcdHID;
  __u8   bCountryCode;
  __u8   bNumReports;
  __u8   bReportType;
  __le16 wReportLength;
} __attribute__ ((packed));

struct driver_data {
  u8 last_request_type;
  u8 last_request;
  struct usb_request* ep0_request;
  struct usb_ep* ep_in;
  struct usb_request* ep_in_request;
  struct usb_ep* ep_out;
  struct usb_request* ep_out_request;
};

#include "hid_opg_gpio.h"

//#include "hid_opg_ps4.h"
//#include "hid_opg_xbo.h"
//#include "hid_opg_ngm.h"
#include "hid_opg_rbg.h"

MODULE_LICENSE("Dual BSD/GPL");

#if !defined(NO_IDX_PRODUCT)
# define NO_IDX_PRODUCT 0
#endif

#if !defined(NO_IDX_SERIAL)
# define NO_IDX_SERIAL 0
#endif

#define USB_BUFSIZ 1024

struct usb_device_descriptor device_desc = {
  .bLength            = USB_DT_DEVICE_SIZE,
  .bDescriptorType    = USB_DT_DEVICE,
  .bcdUSB             = OPG_USB_VERSION,
  .bDeviceClass       = OPG_DEVICE_CLASS,
  .bDeviceSubClass    = OPG_DEVICE_SUB_CLASS,
  .bDeviceProtocol    = OPG_DEVICE_PROTOCOL,
  .bMaxPacketSize0    = 64,
  .idVendor           = cpu_to_le16(OPG_VENDOR_ID),
  .idProduct          = cpu_to_le16(OPG_PRODUCT_ID),
  .bcdDevice          = cpu_to_le16(0x0100),
  .iManufacturer      = IDX_MANUFACTURER,
  .iProduct           = NO_IDX_PRODUCT ? 0 : IDX_PRODUCT,
  .iSerialNumber      = NO_IDX_SERIAL ? 0 : IDX_SERIAL,
  .bNumConfigurations = 1,
};

struct usb_string_descriptor string_desc_lang = {
  .bLength         = 4,
  .bDescriptorType = USB_DT_STRING,
  .wData           = { cpu_to_le16(0x0409) },
};

struct ep_caps {
  u8 address;
  unsigned type_iso:1;
  unsigned type_bulk:1;
  unsigned type_int:1;
  unsigned dir_in:1;
  unsigned dir_out:1;
};

void estimate_ep_caps(const char* name, struct ep_caps* caps) {
  int index = 2;
  memset(caps, 0, sizeof(struct ep_caps));

  // unknown convention.
  if (name[0] != 'e' || name[1] != 'p')
    return;

  // ep[1-9]?.*: address restriction
  if ('1' <= name[index] && name[index] <= '9') {
    caps->address = name[index] - '0';
    index++;
  }

  if (name[index] == 'i') {
    // ep[1-9]?in.*
    index += 2;
    caps->dir_in = 1;
  } else if (name[index] == 'o') {
    // ep[1-9]?out.*
    index += 3;
    caps->dir_out = 1;
  } else {
    // ep[1-9]?.*
    caps->dir_in = 1;
    caps->dir_out = 1;
  }

  if (!name[index]) {
    // ep[1-9]?{in|out}?
    caps->type_iso = 1;
    caps->type_bulk = 1;
    caps->type_int = 1;
    return;
  }

  // unknown convention.
  if (name[index] != '-')
    return;
  index++;

  if (name[index] == 'i') {
    if (name[index + 1] == 's') {
      // ep[1-9]?{in|out}?-iso
      caps->type_iso = 1;
    } else {
      // ep[1-9]?{in|out}?-int
      caps->type_int = 1;
    }
  } else if (name[index] == 'b') {
    // ep[1-9]?{in|out}?-bulk
    caps->type_bulk = 1;
    caps->type_int = 1;  // Usually bulk can handle int.
  }
  return;
}

static int get_descriptor(
    struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  u16 w_value = le16_to_cpu(r->wValue);
  u8 type = w_value >> 8;
  u8 index = w_value & 0xff;

  switch (type) {
    case USB_DT_DEVICE:
      if (device_desc.bMaxPacketSize0 > gadget->ep0->maxpacket) {
        device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;
        printk("%s: shrink ep0 packet size %d\n",
            opg_driver_name, device_desc.bMaxPacketSize0);
      }
      memcpy(data->ep0_request->buf, &device_desc, sizeof(device_desc));
      return sizeof(device_desc);
    case USB_DT_CONFIG:
      memcpy(data->ep0_request->buf, &opg_config_desc, sizeof(opg_config_desc));
      return sizeof(opg_config_desc);
    case USB_DT_STRING:
      if (index) {
        struct usb_string_descriptor* buf = data->ep0_request->buf;
        const char* string = opg_get_string(index);
        size_t length;
        if (!string) {
          printk("%s: unknown string index %d\n", opg_driver_name, index);
          break;
        }
        buf->bDescriptorType = USB_DT_STRING;
        for (length = 0; string[length]; ++length)
          buf->wData[length] = cpu_to_le16(string[length]);
        buf->bLength = length * 2 + 2;
        return buf->bLength;
      }
      memcpy(data->ep0_request->buf, &string_desc_lang,
          string_desc_lang.bLength);
      return string_desc_lang.bLength;
#if OPG_DEVICE_CLASS != USB_CLASS_VENDOR_SPEC
    case USB_DT_HID_REPORT:
      memcpy(data->ep0_request->buf, opg_hid_report, sizeof(opg_hid_report));
      return sizeof(opg_hid_report);
#endif
    default:
      printk("%s: unknown descriptor %02x, ignoring\n", opg_driver_name, type);
      break;
  }
  return -EOPNOTSUPP;
}

static void setup_complete(struct usb_ep* ep, struct usb_request* r) {
  struct driver_data* data = ep->driver_data;
  if (r->status) {
    printk("%s: failed on setup; status=%d, bRequestType=%02x, bRequest=%02x\n",
        opg_driver_name, r->status, data->last_request_type,
        data->last_request);
  }
}

static void in_report_complete(struct usb_ep* ep, struct usb_request* r) {
  int result;

  if (r->status) {
    printk("%s: failed to send an in-data report, suspending\n",
        opg_driver_name);
    usb_ep_fifo_flush(ep);
    return;
  }

  opg_update_report();
  memcpy(r->buf, opg_report, sizeof(opg_report));
  r->length = sizeof(opg_report);
  r->zero = 0;
  result = usb_ep_queue(ep, r, GFP_ATOMIC);
  if (result < 0)
    printk("%s: failed to queue an in-data report\n", opg_driver_name);
}

#if defined(USE_EP_OUT)
static void out_report_complete(struct usb_ep* ep, struct usb_request* r) {
  printk("%s: not impl, out_report_complete\n", opg_driver_name);
}
#endif

static int setup(struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  u16 w_length = le16_to_cpu(r->wLength);
  int value = -EOPNOTSUPP;

  data->ep0_request->zero = 0;
  data->ep0_request->complete = setup_complete;
  data->ep0_request->length = 0;
  data->ep0_request->status = 0;

  value = opg_setup(gadget, r);
  if (value == -EOPNOTSUPP) {
    int type = r->bRequestType & USB_TYPE_MASK;
    if (type == USB_TYPE_STANDARD) switch (r->bRequest) {
      case USB_REQ_GET_DESCRIPTOR:
        value = get_descriptor(gadget, r);
        break;
      case USB_REQ_SET_CONFIGURATION:
        if (data->ep_in && !data->ep_in_request) {
          data->ep_in_request = usb_ep_alloc_request(data->ep_in, GFP_KERNEL);
          if (data->ep_in_request) {
            data->ep_in_request->buf =
                kmalloc(data->ep_in->desc->wMaxPacketSize, GFP_KERNEL);
            if (data->ep_in_request->buf)
              usb_ep_enable(data->ep_in);
          }
        }

#if defined(USE_EP_OUT)
        if (data->ep_out && !data->ep_out_request) {
          data->ep_out_request = usb_ep_alloc_request(data->ep_out, GFP_KERNEL);
          if (data->ep_out_request) {
            data->ep_out_request->buf =
               kmalloc(data->ep_out->desc->wMaxPacketSize, GFP_KERNEL);
            if (data->ep_out_request->buf)
              usb_ep_enable(data->ep_out);
          }
        }
#endif

        if (data->ep_in_request && data->ep_in_request->buf) {
          data->ep_in_request->status = 0;
          data->ep_in_request->zero = 0;
          data->ep_in_request->complete = in_report_complete;
          data->ep_in_request->length = data->ep_in->desc->wMaxPacketSize;
          in_report_complete(data->ep_in, data->ep_in_request);
        } else {
          printk("%s: failed to setup data-in endpoint\n", opg_driver_name);
          value = -ENOMEM;
          break;
        }

#if defined(USE_EP_OUT)
        if (data->ep_out_request && data->ep_out_request->buf) {
          data->ep_out_request->status = 0;
          data->ep_out_request->zero = 0;
          data->ep_out_request->complete = out_report_complete;
          data->ep_out_request->length = data->ep_out->desc->wMaxPacketSize;
        } else {
          printk("%s: failed to setup data-out endpoint\n", opg_driver_name);
          value = -ENOMEM;
          break;
        }
#endif
        value = w_length;
        break;
      default:
        printk("%s: standard setup not impl: %02x-%02x\n",
            opg_driver_name, r->bRequestType, r->bRequest);
        break;
    } else if (type == USB_TYPE_CLASS) switch (r->bRequest) {
      case HID_REQ_GET_REPORT:
        opg_update_report();
        memcpy(data->ep0_request->buf, opg_report, sizeof(opg_report));
        value = sizeof(opg_report);
        break;
      case HID_REQ_SET_REPORT:
        value = w_length;
        break;
      case HID_REQ_SET_IDLE:
        value = w_length;
        break;
      default:
        printk("%s: hid class setup not impl: %02x-%02x\n",
            opg_driver_name, r->bRequestType, r->bRequest);
        break;
    } else {
        printk("%s: setup not impl: %02x-%02x\n",
            opg_driver_name, r->bRequestType, r->bRequest);
    }
  }

  if (value > 0) {
    data->ep0_request->length = min((u16)value, w_length);
    data->ep0_request->zero = value > w_length;
    data->last_request_type = r->bRequestType;
    data->last_request = r->bRequest;
    value = usb_ep_queue(gadget->ep0, data->ep0_request, GFP_ATOMIC);
    if (value < 0)
      printk("%s: usb_ep_queue returns a negative value\n", opg_driver_name);
  }

  return value;
}

struct usb_ep* find_int_ep(struct usb_gadget* gadget, int maxpacket, int in) {
  struct usb_ep* ep;
  list_for_each_entry(ep, &gadget->ep_list, ep_list) {
    struct ep_caps caps;
    estimate_ep_caps(ep->name, &caps);
    if (caps.type_int) {
      if (caps.dir_in && in && ep->maxpacket >= maxpacket) {
        ep->address = USB_DIR_IN | caps.address;
        return ep;
      } else if (caps.dir_out && !in && ep->maxpacket >= maxpacket) {
        ep->address = USB_DIR_OUT | caps.address;
        return ep;
      }
    }
  }
  return NULL;
}

static int bind(struct usb_gadget* gadget) {
  struct driver_data* data = kzalloc(sizeof(struct driver_data), GFP_KERNEL);
  if (!data)
    return -ENOMEM;
  set_gadget_data(gadget, data);

  // Initialize EP0 for setup.
  data->ep0_request = usb_ep_alloc_request(gadget->ep0, GFP_KERNEL);
  if (!data->ep0_request)
    return -ENOMEM;
  data->ep0_request->buf = kmalloc(USB_BUFSIZ, GFP_KERNEL);
  if (!data->ep0_request->buf)
    return -ENOMEM;
  gadget->ep0->driver_data = data;

  // Claim endpoints for interrupt transfer.
  data->ep_in = find_int_ep(gadget, opg_config_desc.ep_in.wMaxPacketSize, 1);
  if (data->ep_in) {
    data->ep_in->driver_data = data;
    data->ep_in->desc =
      (struct usb_endpoint_descriptor*)&opg_config_desc.ep_in;
    opg_config_desc.ep_in.bEndpointAddress = data->ep_in->address;
  } else {
    printk("%s: failed to allocate ep-in\n", opg_driver_name);
    return -EOPNOTSUPP;
  }
#if defined(USE_EP_OUT)
  data->ep_out = find_int_ep(gadget, opg_config_desc.ep_out.wMaxPacketSize, 0);
  if (data->ep_out) {
    data->ep_out->driver_data = data;
    data->ep_out->desc =
      (struct usb_endpoint_descriptor*)&opg_config_desc.ep_out;
    opg_config_desc.ep_out.bEndpointAddress = data->ep_out->address;
  } else {
    printk("%s: failed to allocate ep-out, ignoring\n", opg_driver_name);
  }
#endif
  return 0;
}

static void unbind(struct usb_gadget* gadget) {
  struct driver_data* data = get_gadget_data(gadget);
  if (!data)
    return;

  if (data->ep_in) {
    usb_ep_disable(data->ep_in);
    data->ep_in->driver_data = NULL;
    data->ep_in->desc = NULL;
    if (data->ep_in_request) {
      if (data->ep_in_request->buf)
        kfree(data->ep_in_request->buf);
      usb_ep_free_request(data->ep_in, data->ep_in_request);
    }
  }
#if defined(USE_EP_OUT)
  if (data->ep_out) {
    usb_ep_disable(data->ep_out);
    data->ep_out->driver_data = NULL;
    data->ep_out->desc = NULL;
    if (data->ep_out_request) {
      if (data->ep_out_request->buf)
        kfree(data->ep_out_request->buf);
      usb_ep_free_request(data->ep_out, data->ep_out_request);
    }
  }
#endif

  if (data->ep0_request) {
    if (data->ep0_request->buf)
      kfree(data->ep0_request->buf);
    usb_ep_free_request(gadget->ep0, data->ep0_request);
  }

  kfree(data);
  set_gadget_data(gadget, NULL);
}

static void disconnect(struct usb_gadget* gadget) {
  printk("%s: disconnect\n", opg_driver_name);
  // TODO: finalize endpoints for interrupt in/out.
}

static void not_impl(struct usb_gadget* gadget) {
  printk("%s: not impl\n", opg_driver_name);
}

static struct usb_gadget_driver driver = {
  .function   = "USB Gadget Test Driver",
  .max_speed  = USB_SPEED_HIGH,
  .unbind     = unbind,
  .setup      = setup,
  .disconnect = disconnect,
  .suspend    = not_impl,
  .resume     = not_impl,
  .driver = { .owner = THIS_MODULE },
};

static int __init init(void) {
  driver.function = opg_driver_name;
  return usb_gadget_probe_driver(&driver, bind);
}
module_init(init);

static void __exit cleanup(void) {
  usb_gadget_unregister_driver(&driver);
}
module_exit(cleanup);
