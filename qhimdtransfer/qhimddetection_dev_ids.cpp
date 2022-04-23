#include "qhimddetection_dev_ids.h"

const char *identify_usb_device(int vid, int pid) {
    if (vid == 0x054c && pid == 0x017f) {
        return "Sony MZ-NH1";
    }

    if (vid == 0x054c && pid == 0x0181) {
        return "Sony MZ-NH3D";
    }

    if (vid == 0x054c && pid == 0x0183) {
        return "Sony MZ-NH900";
    }

    if (vid == 0x054c && pid == 0x0185) {
        return "Sony MZ-NH700 / MZ-NH800";
    }

    if (vid == 0x054c && pid == 0x0187) {
        return "Sony MZ-NH600(D)";
    }

    if (vid == 0x054c && pid == 0x018b) {
        return "Sony LAM-3";
    }

    if (vid == 0x054c && pid == 0x01ea) {
        return "Sony MZ-DH10P";
    }

    if (vid == 0x054c && pid == 0x021a) {
        return "Sony MZ-RH10";
    }

    if (vid == 0x054c && pid == 0x021c) {
        return "Sony MZ-RH910 / MZ-M10";
    }

    if (vid == 0x054c && pid == 0x022d) {
        return "Sony CMT-AH10";
    }

    if (vid == 0x054c && pid == 0x023d) {
        return "Sony DS-HMD1";
    }

    if (vid == 0x054c && pid == 0x0287) {
        return "Sony MZ-RH1";
    }

    if (vid == 0x04da && pid == 0x23b3) {
        return "Panasonic SJ-MR250 (NetMD)";
    }

    if (vid == 0x04da && pid == 0x23b6) {
        return "Panasonic SJ-MR270 (NetMD)";
    }

    if (vid == 0x04dd && pid == 0x7202) {
        return "Sharp IM-MT880H/MT899H (NetMD)";
    }

    if (vid == 0x04dd && pid == 0x9013) {
        return "Sharp IM-DR400/DR410 (NetMD)";
    }

    if (vid == 0x04dd && pid == 0x9014) {
        return "Sharp IM-DR80/DR420/DR580 or Kenwood DMC-S9NET (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0034) {
        return "Sony PCLK-XX (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0036) {
        return "Sony NetMD (unknown model) (NetMD)";
    }

    if (vid == 0x054c && pid == 0x006f) {
        return "Sony NW-E7 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0075) {
        return "Sony MZ-N1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x007c) {
        return "Sony (unknown model) (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0080) {
        return "Sony LAM-1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0081) {
        return "Sony MDS-JB980/MDS-NT1/MDS-JE780 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0084) {
        return "Sony MZ-N505 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0085) {
        return "Sony MZ-S1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0086) {
        return "Sony MZ-N707 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x008e) {
        return "Sony CMT-C7NT (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0097) {
        return "Sony PCGA-MDN1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00ad) {
        return "Sony CMT-L7HD (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00c6) {
        return "Sony MZ-N10 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00c7) {
        return "Sony MZ-N910 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00c8) {
        return "Sony MZ-N710/NE810/NF810 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00c9) {
        return "Sony MZ-N510/NF610 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00ca) {
        return "Sony MZ-NE410/DN430/NF520D (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00e7) {
        return "Sony CMT-M333NT/M373NT (NetMD)";
    }

    if (vid == 0x054c && pid == 0x00eb) {
        return "Sony MZ-NE810/NE910/DN430 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0101) {
        return "Sony LAM-10 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0113) {
        return "Aiwa AM-NX1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0119) {
        return "Sony CMT-SE9 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x011a) {
        return "Sony CMT-SE7 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x013f) {
        return "Sony MDS-S500 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x014c) {
        return "Aiwa AM-NX9 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x017e) {
        return "Sony MZ-NH1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0180) {
        return "Sony MZ-NH3D (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0182) {
        return "Sony MZ-NH900 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0184) {
        return "Sony MZ-NH700/MZ-NH800 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0186) {
        return "Sony MZ-NH600/MZ-NH600D (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0187) {
        return "Sony MZ-NH600D (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0188) {
        return "Sony MZ-N920 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x018a) {
        return "Sony LAM-3 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x01e9) {
        return "Sony MZ-DH10P (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0219) {
        return "Sony MZ-RH10 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x021b) {
        return "Sony MZ-RH710/MZ-RH910/MZ-M10 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x021d) {
        return "Sony CMT-AH10 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x022c) {
        return "Sony CMT-AH10 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x023c) {
        return "Sony DS-HMD1 (NetMD)";
    }

    if (vid == 0x054c && pid == 0x0286) {
        return "Sony MZ-RH1 (NetMD)";
    }

    if (vid == 0x0b28 && pid == 0x1004) {
        return "Kenwood MDX-J9 (NetMD)";
    }

    return NULL;
}
