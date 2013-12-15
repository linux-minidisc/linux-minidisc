#ifndef QHIMDFORMATDIALOG_H
#define QHIMDFORMATDIALOG_H

#include <QDialog>

namespace Ui {
    class QHiMDFormatDialog;
}

class QHiMDFormatDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(QHiMDFormatDialog)
public:
    explicit QHiMDFormatDialog(QWidget *parent = 0);
    virtual ~QHiMDFormatDialog();

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::QHiMDFormatDialog *m_ui;
};

#endif // QHIMDFORMATDIALOG_H
