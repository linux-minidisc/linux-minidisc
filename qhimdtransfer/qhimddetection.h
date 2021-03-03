#ifndef QHIMDDETECTION_H
#define QHIMDDETECTION_H

#include <QObject>
#include <QList>
#include <QString>
#include <qmddevice.h>

// known vendor IDs
#define SONY 0x054c
#define SHARP 0x4dd

// known himd-mode product IDs
#define MZ_NH1_HIMD 0x017f
#define MZ_NH3D_HIMD 0x0181
#define MZ_NH900_HIMD 0x0183
#define MZ_NH700_HIMD 0x0185
#define MZ_NH600_HIMD 0x0187
#define LAM_3_HIMD 0x018b
#define MZ_DH10P_HIMD 0x01ea
#define MZ_RH10_HIMD 0x021a
#define MZ_RH910_HIMD 0x021c       // or MZ-M10
#define CMT_AH10_HIMD 0x022d
#define DS_HMD1_HIMD 0x023d
#define MZ_RH1_HIMD 0x0287

// known Sony/Aiwa netmd-mode product IDs
#define PCLK_XX 0x34
#define UNKNOWN_A 0x36
#define MZ_N1 0x75
#define UNKNOWN_B 0x7c
#define LAM_1 0x80
#define MDS_JE780 0x81            // or MDS-JE980
#define MZ_N505 0x84
#define MZ_S1 0x85
#define MZ_N707 0x86
#define CMT_C7NT 0x8e
#define PCGA_MDN1 0x97
#define CMT_L7HD 0xad
#define MZ_N10 0xc6
#define MZ_N910 0xc7
#define MZ_N710 0xc8               // or MZ-NE810/NF810
#define MZ_N510 0xc9               // or MZ-NF610
#define MZ_NE410 0xca              // or MZ-DN430/NF520
#define MZ_NE810 0xeb              // or MZ-NE910
#define CMT_M333NT 0xe7            // or CMT-M373NT
#define LAM_10 0x101
#define AIWA_AM_NX1 0x113
#define AIWA_AM_NX9 0x14c
#define MZ_NH1 0x17e
#define MZ_NH3D 0x180
#define MZ_NH900 0x182
#define MZ_NH700 0x184             // or MZ-NH800
#define MZ_NH600 0x186             // or MZ-NH600D
#define MZ_N920 0x188
#define LAM_3 0x18a
#define MZ_DH10P 0x1e9
#define MZ_RH10 0x219
#define MZ_RH910 0x21b             // or MZ-M10
#define CMT_AH10_A 0x21d
#define CMT_AH10_B 0x22c
#define DS_HMD1 0x23c
#define MZ_RH1 0x286

// known Sharp netmd-mode product IDs
#define IM_MT880H 0x7202            // or IM-MT899H
#define IM_DR400 0x9013             // or IM-DR410
#define IM_DR80 0x9014              // or IM-DR420/DR580 / Kenwood DMC-S9NET

const char * identify_usb_device(int vid, int pid);

typedef QList<QMDDevice *> QMDDevicePtrList;

class QHiMDDetection : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(QHiMDDetection)

protected:
    QMDDevicePtrList dlist;
    netmd_device * dev_list;
public:
    explicit QHiMDDetection(QObject *parent = 0);
    virtual ~QHiMDDetection();
    virtual void clearDeviceList();
    virtual void cleanup_netmd_list();
    void rescan_netmd_devices();
    void scan_for_minidisc_devices();
    virtual void scan_for_himd_devices(){}
    virtual void remove_himddevice(QString path);
    void scan_for_netmd_devices();
    QMDDevice *find_by_path(QString path);
    QMDDevice *find_by_name(QString name);
    virtual QString mountpoint(QMDDevice *dev);

signals:
    void deviceListChanged(QMDDevicePtrList list);
};

QHiMDDetection * createDetection(QObject * parent = NULL);

#endif // QHIMDDETECTION_H
