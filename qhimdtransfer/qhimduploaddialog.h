#ifndef QHIMDUPLOADDIALOG_H
#define QHIMDUPLOADDIALOG_H

#include <QtGui/QDialog>
#include "qhimdmodel.h"

namespace Ui {
    class QHiMDUploadDialog;
}

class QHiMDUploadDialog : public QDialog {
    Q_OBJECT

public:
    explicit QHiMDUploadDialog(QWidget *parent = 0);
    virtual ~QHiMDUploadDialog();
    bool upload_canceled() { return canceled; }

    void init(int trackcount, int totalblocks);
    void starttrack(const QHiMDTrack & trk, const QString & title);
    void blockTransferred();
    void trackFailed(const QString & errmsg);
    void trackSucceeded();
    void finished();

private:
    Ui::QHiMDUploadDialog *m_ui;
    int allblocks, allfinished;
    int tracknum;
    int thisfileblocks, thisfilefinished;
    int scount, fcount;
    bool canceled;

private slots:
    /* UI slots */
    void on_close_button_clicked();
    void on_cancel_button_clicked();
    void on_details_button_toggled(bool checked);
};

#endif // QHIMDUPLOADDIALOG_H
