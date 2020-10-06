/******************************************************************************
 * @file     descriptors.c
 * @brief    USBD descriptors
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include "M071Q_M071V.h"
#include "HID_Transfer_and_MSC.h"
#include "massstorage.h"

/*!<USB HID Report Descriptor */
uint8_t HID_DeviceReportDescriptor[] =
{
    0x06, 0x00, 0xFF,   // Usage Page = 0xFF00 (Vendor Defined Page 1)
    0x09, 0x01,         // Usage (Vendor Usage 1)
    0xA1, 0x01,         // Collection (Application)
    0x19, 0x01,         // Usage Minimum
    0x29, 0x40,         // Usage Maximum //64 input usages total (0x01 to 0x40)
    0x15, 0x00,         // Logical Minimum (data bytes in the report may have minimum value = 0x00)
    0x26, 0xFF, 0x00,   // Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
    0x75, 0x08,         // Report Size: 8-bit field size
    0x95, 0x40,         // Report Count: Make sixty-four 8-bit fields (the next time the parser hits
    // an "Input", "Output", or "Feature" item)
    0x81, 0x00,         // Input (Data, Array, Abs): Instantiates input packet fields based on the
    // above report size, count, logical min/max, and usage.
    0x19, 0x01,         // Usage Minimum
    0x29, 0x40,         // Usage Maximum //64 output usages total (0x01 to 0x40)
    0x91, 0x00,         // Output (Data, Array, Abs): Instantiates output packet fields. Uses same
    // report size and count as "Input" fields, since nothing new/different was
    // specified to the parser since the "Input" item.
    0xC0                // End Collection
};

/*----------------------------------------------------------------------------*/
/*!<USB Device Descriptor */
const uint8_t gu8DeviceDescriptor[] =
{
    LEN_DEVICE,             /* bLength */
    DESC_DEVICE,            /* bDescriptorType */
#ifdef SUPPORT_LPM
    0x01, 0x02,     /* bcdUSB >= 0x0201 to support LPM */
#else
    0x10, 0x01,     /* bcdUSB */
#endif
    0x00,                   /* bDeviceClass */
    0x00,                   /* bDeviceSubClass */
    0x00,                   /* bDeviceProtocol */
    EP0_MAX_PKT_SIZE,       /* bMaxPacketSize0 */
    /* idVendor */
    USBD_VID & 0x00FF,
    (USBD_VID & 0xFF00) >> 8,
    /* idProduct */
    USBD_PID & 0x00FF,
    (USBD_PID & 0xFF00) >> 8,
    0x00, 0x00,             /* bcdDevice */
    0x01,                   /* iManufacture */
    0x02,                   /* iProduct */
    0x03,                   /* iSerialNumber */
    0x01                    /* bNumConfigurations */
};

/*!<USB Configure Descriptor */
const uint8_t gu8ConfigDescriptor[] =
{
    LEN_CONFIG,     /* bLength */
    DESC_CONFIG,    /* bDescriptorType */
    /* wTotalLength */
    0x40,
    0x00,
    0x02,           /* bNumInterfaces */
    0x01,           /* bConfigurationValue */
    0x00,           /* iConfiguration */
    0x80 | (USBD_SELF_POWERED << 6) | (USBD_REMOTE_WAKEUP << 5),/* bmAttributes */
    USBD_MAX_POWER, /* MaxPower */

    /* I/F descr: HID */
    LEN_INTERFACE,  /* bLength */
    DESC_INTERFACE, /* bDescriptorType */
    0x00,           /* bInterfaceNumber */
    0x00,           /* bAlternateSetting */
    0x02,           /* bNumEndpoints */
    0x03,           /* bInterfaceClass */
    0x00,           /* bInterfaceSubClass */
    0x00,           /* bInterfaceProtocol */
    0x00,           /* iInterface */

    /* HID Descriptor */
    LEN_HID,        /* Size of this descriptor in UINT8s. */
    DESC_HID,       /* HID descriptor type. */
    0x10, 0x01,     /* HID Class Spec. release number. */
    0x00,           /* H/W target country. */
    0x01,           /* Number of HID class descriptors to follow. */
    DESC_HID_RPT,   /* Descriptor type. */
    /* Total length of report descriptor. */
    sizeof(HID_DeviceReportDescriptor) & 0x00FF,
    (sizeof(HID_DeviceReportDescriptor) & 0xFF00) >> 8,

    /* EP Descriptor: interrupt in. */
    LEN_ENDPOINT,                               /* bLength */
    DESC_ENDPOINT,                              /* bDescriptorType */
    (INT_IN_EP_NUM | EP_INPUT),                 /* bEndpointAddress */
    EP_INT,                                     /* bmAttributes */
    /* wMaxPacketSize */
    EP2_MAX_PKT_SIZE & 0x00FF,
    (EP2_MAX_PKT_SIZE & 0xFF00) >> 8,
    HID_DEFAULT_INT_IN_INTERVAL,                /* bInterval */

    /* EP Descriptor: interrupt out. */
    LEN_ENDPOINT,                               /* bLength */
    DESC_ENDPOINT,                              /* bDescriptorType */
    (INT_OUT_EP_NUM | EP_OUTPUT),               /* bEndpointAddress */
    EP_INT,                                     /* bmAttributes */
    /* wMaxPacketSize */
    EP3_MAX_PKT_SIZE & 0x00FF,
    (EP3_MAX_PKT_SIZE & 0xFF00) >> 8,
    HID_DEFAULT_INT_IN_INTERVAL,                /* bInterval */

    /* MSC Descriptor */
    /* const BYTE cbyInterfaceDescriptor[LEN_INTERFACE] = */
    LEN_INTERFACE,                              // bLength
    DESC_INTERFACE,                             // bDescriptorType
    0x01,                                       // bInterfaceNumber
    0x00,                                       // bAlternateSetting
    0x02,                                       // bNumEndpoints
    0x08,                                       // bInterfaceClass
    0x06,                                       // bInterfaceSubClass
    0x50,                                       // bInterfaceProtocol
    0x00,                                       // iInterface

    /* const BYTE cbyEndpointDescriptor1[LEN_ENDPOINT] = */
    LEN_ENDPOINT,                               // bLength
    DESC_ENDPOINT,                              // bDescriptorType
    (EP_INPUT | BULK_IN_EP_NUM),                // bEndpointAddress
    EP_BULK,                                    // bmAttributes
    EP4_MAX_PKT_SIZE, 0x00,                     // wMaxPacketSize
    0x00,                                       // bInterval

    /* const BYTE cbyEndpointDescriptor2[LEN_ENDPOINT] = */
    LEN_ENDPOINT,                               // bLength
    DESC_ENDPOINT,                              // bDescriptorType
    (EP_OUTPUT | BULK_OUT_EP_NUM),              // bEndpointAddress
    EP_BULK,                                    // bmAttributes
    EP5_MAX_PKT_SIZE, 0x00,                     // wMaxPacketSize
    0x00                                        // bInterval
};

/*!<USB Language String Descriptor */
const uint8_t gu8StringLang[4] =
{
    4,              /* bLength */
    DESC_STRING,    /* bDescriptorType */
    0x09, 0x04
};

/*!<USB Vendor String Descriptor */
const uint8_t gu8VendorStringDesc[] =
{
    16,
    DESC_STRING,
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

/*!<USB Product String Descriptor */
const uint8_t gu8ProductStringDesc[] =
{
    22,             /* bLength          */
    DESC_STRING,    /* bDescriptorType  */
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

uint8_t gu8StringSerial[26] =
{
    26,             // bLength
    DESC_STRING,    // bDescriptorType
    'A', 0, '0', 0, '2', 0, '0', 0, '1', 0, '5', 0, '0', 0, '8', 0, '2', 0, '1', 0, '0', 0, '1', 0
};

const uint8_t *gpu8UsbString[4] =
{
    gu8StringLang,
    gu8VendorStringDesc,
    gu8ProductStringDesc,
    gu8StringSerial
};

#ifdef SUPPORT_LPM
const uint8_t gu8BosDescriptor[] =
{
    LEN_BOS,        /* bLength */
    DESC_BOS,       /* bDescriptorType */
    /* wTotalLength */
    0x0C & 0x00FF,
    (0x0C & 0xFF00) >> 8,
    0x01,           /* bNumDeviceCaps */

    /* Device Capability */
    LEN_DEVCAP,     /* bLength */
    DESC_DEVCAP,/* bDescriptorType */
    0x02,  /* bDevCapabilityType, 0x02 is USB 2.0 Extension */
    0x06, 0x04, 0x00, 0x00  /* bmAttributes, 32 bits */
                            /* bit 0 : Reserved. Must 0. */
                            /* bit 1 : 1 to support LPM. */
                            /* bit 2 : 1 to support BSL & Alternat HIRD. */
                            /* bit 3 : 1 to recommend Baseline BESL. */
                            /* bit 4 : 1 to recommand Deep BESL. */
                            /* bit 11:8 : Recommend Baseline BESL value. Ignore by bit3 is zero. */
                            /* bit 15:12 : Recommend Deep BESL value. Ignore by bit4 is zero. */
                            /* bit 31:16 : Reserved. Must 0. */
};
#endif

const uint8_t *gpu8UsbHidReport[3] =
{
    HID_DeviceReportDescriptor,
    NULL,
    NULL
};

const uint32_t gu32UsbHidReportLen[3] =
{
    sizeof(HID_DeviceReportDescriptor),
    0,
    0
};

const uint32_t gu32ConfigHidDescIdx[3] =
{
    (LEN_CONFIG + LEN_INTERFACE),
    0,
    0
};

const S_USBD_INFO_T gsInfo =
{
    gu8DeviceDescriptor,
    gu8ConfigDescriptor,
    gpu8UsbString,
    gpu8UsbHidReport,
    gu32UsbHidReportLen,
    gu32ConfigHidDescIdx,
#ifdef SUPPORT_LPM
    gu8BosDescriptor
#else
    NULL
#endif
};


/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
