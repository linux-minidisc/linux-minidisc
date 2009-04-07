#include "qhimdformatdialog.h"
#include "ui_qhimdformatdialog.h"

QHiMDFormatDialog::QHiMDFormatDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::QHiMDFormatDialog)
{
    m_ui->setupUi(this);
}

QHiMDFormatDialog::~QHiMDFormatDialog()
{
    delete m_ui;
}

void QHiMDFormatDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
