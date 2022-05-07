#ifndef QHIMDUPLOADDIALOG_H
#define QHIMDUPLOADDIALOG_H

#include <QDialog>
#include "qmdtrack.h"

namespace Ui {
    class QHiMDUploadDialog;
}

class QHiMDUploadDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode {
        UPLOAD = 0,
        DOWNLOAD = 1,
    };

    explicit QHiMDUploadDialog(QWidget *parent, enum Mode mode);
    virtual ~QHiMDUploadDialog();
    bool was_canceled() { return canceled; }

    void init(int trackcount, int totalblocks);
    void starttrack(const QMDTrack & trk, const QString & title);
    void starttrack(const QString &filename);
    void blockTransferred(int count=1);
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
    enum Mode mode;
    QString currentFilename;

private slots:
    /* UI slots */
    void on_close_button_clicked();
    void on_cancel_button_clicked();
    void on_details_button_toggled(bool checked);
};

#endif // QHIMDUPLOADDIALOG_H
