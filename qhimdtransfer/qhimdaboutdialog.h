#ifndef QHIMDABOUTDIALOG_H
#define QHIMDABOUTDIALOG_H

#include <QDialog>

namespace Ui {
    class QHiMDAboutDialog;
}

class QHiMDAboutDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(QHiMDAboutDialog)
public:
    explicit QHiMDAboutDialog(QWidget *parent = 0);
    virtual ~QHiMDAboutDialog();

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::QHiMDAboutDialog *m_ui;
};

#endif // QHIMDABOUTDIALOG_H
