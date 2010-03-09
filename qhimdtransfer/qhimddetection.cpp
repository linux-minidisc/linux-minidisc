#include "qhimddetection.h"
#include "ui_qhimddetection.h"

QHiMDDetection::QHiMDDetection(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::QHiMDDetection)
{
    m_ui->setupUi(this);
}

QHiMDDetection::~QHiMDDetection()
{
    delete m_ui;
}

void QHiMDDetection::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void QHiMDDetection::closeEvent(QCloseEvent *event)
{
    disconnect();
}
