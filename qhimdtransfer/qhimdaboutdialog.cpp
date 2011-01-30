#include "qhimdaboutdialog.h"
#include "ui_qhimdaboutdialog.h"

QHiMDAboutDialog::QHiMDAboutDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::QHiMDAboutDialog)
{
    m_ui->setupUi(this);
    m_ui->VersionString->setText(VER);
}

QHiMDAboutDialog::~QHiMDAboutDialog()
{
    delete m_ui;
}

void QHiMDAboutDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
