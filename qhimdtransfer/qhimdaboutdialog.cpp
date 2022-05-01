#include "qhimdaboutdialog.h"
#include "ui_qhimdaboutdialog.h"

#include "qhimdtransfer_config.h"

#include <QDesktopServices>

QHiMDAboutDialog::QHiMDAboutDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::QHiMDAboutDialog)
{
    m_ui->setupUi(this);
    m_ui->VersionString->setText(QHIMDTRANSFER_VERSION);
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

void QHiMDAboutDialog::onLinkActivated(const QString &url)
{
    QDesktopServices::openUrl(url);
}
