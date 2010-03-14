#ifndef QHIMDDETECTION_H
#define QHIMDDETECTION_H

#include <QtGui/QDialog>

#define SONY 0x054c         //known himd-mode product IDs
#define MZ_NH1 0x017f
#define MZ_NH3D 0x0181
#define MZ_NH900 0x0183
#define MZ_NH700 0x0185
#define MZ_NH600 0x0187
#define LAM_3 0x018a
#define MZ_DH10P 0x01ea
#define MZ_RH10 0x021a
#define MZ_RH910 0x021c
#define CMT_AH10 0x022d
#define DS_HMD1 0x023d
#define MZ_RH1 0x0287

struct himd_device {
                    bool is_busy;
                    QString path;
                    bool md_inserted;
                    QString recorder_name;
                    virtual ~himd_device(){} /* for polymorphic delete */
                    };

class QHiMDDetection : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(QHiMDDetection)

protected:
    QList<himd_device *> device_list;
public:
    explicit QHiMDDetection(QWidget *parent = 0);
    virtual ~QHiMDDetection() {}
    virtual void scan_for_himd_devices(){}
    himd_device *find_by_path(QString path);

protected slots:
    virtual void himd_busy(QString path);
    virtual void himd_idle(QString path);

signals:
    void himd_found(QString path);
    void himd_removed(QString path);
};

QHiMDDetection * createDetection();

#endif // QHIMDDETECTION_H
