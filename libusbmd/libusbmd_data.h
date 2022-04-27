#include "libusbmd.h"
#include <stddef.h>

/* Generated by convert.py from minidisc.devids, do not edit manually */
static const struct minidisc_usb_device_info KNOWN_DEVICES[] = {
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x017f, "Sony MZ-NH1" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x0181, "Sony MZ-NH3D" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x0183, "Sony MZ-NH900" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x0185, "Sony MZ-NH700 / MZ-NH800" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x0187, "Sony MZ-NH600(D)" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x018b, "Sony LAM-3" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x01ea, "Sony MZ-DH10P" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x021a, "Sony MZ-RH10" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x021c, "Sony MZ-RH910 / MZ-M10" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x022d, "Sony CMT-AH10" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x023d, "Sony DS-HMD1" },
    { MINIDISC_USB_DEVICE_TYPE_HIMD, 0x054c, 0x0287, "Sony MZ-RH1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x04da, 0x23b3, "Panasonic SJ-MR250" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x04da, 0x23b6, "Panasonic SJ-MR270" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x04dd, 0x7202, "Sharp IM-MT880H/MT899H" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x04dd, 0x9013, "Sharp IM-DR400/DR410" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x04dd, 0x9014, "Sharp IM-DR80/DR420/DR580 or Kenwood DMC-S9NET" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0034, "Sony PCLK-XX" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0036, "Sony NetMD (unknown model)" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x006f, "Sony NW-E7" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0075, "Sony MZ-N1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x007c, "Sony (unknown model)" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0080, "Sony LAM-1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0081, "Sony MDS-JB980/MDS-NT1/MDS-JE780" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0084, "Sony MZ-N505" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0085, "Sony MZ-S1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0086, "Sony MZ-N707" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x008e, "Sony CMT-C7NT" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0097, "Sony PCGA-MDN1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00ad, "Sony CMT-L7HD" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00c6, "Sony MZ-N10" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00c7, "Sony MZ-N910" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00c8, "Sony MZ-N710/NE810/NF810" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00c9, "Sony MZ-N510/NF610" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00ca, "Sony MZ-NE410/DN430/NF520D" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00e7, "Sony CMT-M333NT/M373NT" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x00eb, "Sony MZ-NE810/NE910/DN430" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0101, "Sony LAM-10" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0113, "Aiwa AM-NX1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0119, "Sony CMT-SE9" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x011a, "Sony CMT-SE7" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x013f, "Sony MDS-S500" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0148, "Sony MDS-A1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x014c, "Aiwa AM-NX9" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x017e, "Sony MZ-NH1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0180, "Sony MZ-NH3D" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0182, "Sony MZ-NH900" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0184, "Sony MZ-NH700/MZ-NH800" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0186, "Sony MZ-NH600/MZ-NH600D" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0187, "Sony MZ-NH600D" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0188, "Sony MZ-N920" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x018a, "Sony LAM-3" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x01e9, "Sony MZ-DH10P" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0219, "Sony MZ-RH10" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x021b, "Sony MZ-RH710/MZ-RH910/MZ-M10" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x021d, "Sony CMT-AH10" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x022c, "Sony CMT-AH10" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x023c, "Sony DS-HMD1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x054c, 0x0286, "Sony MZ-RH1" },
    { MINIDISC_USB_DEVICE_TYPE_NETMD, 0x0b28, 0x1004, "Kenwood MDX-J9" },
    { 0, 0, 0, NULL }, /* sentinel */
};
