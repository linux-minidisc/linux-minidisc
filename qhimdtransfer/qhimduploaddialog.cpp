#include "qhimduploaddialog.h"
#include "ui_qhimduploaddialog.h"

#include <QFileInfo>

void QHiMDUploadDialog::trackFailed(const QString & errmsg)
{
    allfinished -= thisfilefinished;
    allfinished += thisfileblocks;
    m_ui->AllPBar->setValue(allfinished);

    m_ui->failed_text->setText((mode == UPLOAD) ? tr("%1 track(s) could not be uploaded").arg(++fcount)
                                                : tr("%1 track(s) could not be downloaded").arg(++fcount));

    QTreeWidgetItem * ErrorMsg;
    ErrorMsg = new QTreeWidgetItem(0);

    ErrorMsg->setText(0, (mode == UPLOAD) ? tr("Track %1").arg(tracknum) : currentFilename);
    ErrorMsg->setText(1, errmsg);
    m_ui->ErrorList->insertTopLevelItem(0, ErrorMsg);
    m_ui->details_button->setEnabled(true);
}

void QHiMDUploadDialog::trackSucceeded()
{
    /* should do nothing, just to be sure */
    allfinished -= thisfilefinished;
    allfinished += thisfileblocks;
    m_ui->AllPBar->setValue(allfinished);

    m_ui->success_text->setText((mode == UPLOAD) ? tr("%1 track(s) successfully uploaded").arg(++scount)
                                                 : tr("%1 track(s) successfully downloaded").arg(++scount));
}

void QHiMDUploadDialog::finished()
{
    m_ui->curtrack_label->setText(tr("upload finished"));
    /* Prevent shrinking of the box when hiding the indicators */
    m_ui->current->setMinimumSize(m_ui->current->size());
    m_ui->TrkPBar->hide();
    /* set AllPBar to 100% if it is not used during transfer,
     * current netmd uploads doesn´t set the range correctly
     */
    if(m_ui->AllPBar->maximum() == 0)
    {
        m_ui->AllPBar->setMaximum(1);
        m_ui->AllPBar->setValue(1);
    }
    m_ui->curtrack_label->hide();

    m_ui->cancel_button->hide();
    m_ui->close_button->show();

    return;
}

void QHiMDUploadDialog::starttrack(const QMDTrack & trk, const QString & title)
{
    tracknum = trk.tracknum() + 1;
    m_ui->curtrack_label->setText(tr("current track: %1 - %2").arg(tracknum).arg(title));
    thisfileblocks = trk.blockcount();
    thisfilefinished = 0;
    m_ui->TrkPBar->setRange(0, thisfileblocks);
    m_ui->TrkPBar->reset();
}

void QHiMDUploadDialog::starttrack(const QString &filename)
{
    QFileInfo info(filename);

    tracknum = 0;
    currentFilename = filename;
    m_ui->curtrack_label->setText(tr("current file: %1").arg(info.fileName()));
    thisfileblocks = info.size();
    thisfilefinished = 0;
    m_ui->TrkPBar->setRange(0, thisfileblocks);
    m_ui->TrkPBar->reset();
}

void QHiMDUploadDialog::blockTransferred(int count)
{
    m_ui->TrkPBar->setValue((thisfilefinished += count));
    m_ui->AllPBar->setValue((allfinished += count));
}

void QHiMDUploadDialog::init(int trackcount, int totalblocks)
{
    allblocks = totalblocks;
    allfinished = 0;
    m_ui->AllPBar->setRange(0, allblocks);
    m_ui->AllPBar->reset();
    canceled = false;

    scount = fcount = 0;
    m_ui->success_text->setText("");
    m_ui->failed_text->setText("");

    if(!trackcount)
    {
        m_ui->alltrack_label->setText(tr("no tracks selected"));
        finished();
    }
    else
    {
        if (mode == UPLOAD) {
            m_ui->alltrack_label->setText(tr("please wait while uploading %1 track(s)").arg(trackcount));
        } else {
            m_ui->alltrack_label->setText(tr("please wait while downloading %1 track(s)").arg(trackcount));
        }

        /* undo QHiMDUploadDialog::finished */
        m_ui->TrkPBar->show();
        m_ui->curtrack_label->show();
        m_ui->current->setMinimumSize(0,0);
        m_ui->close_button->hide();
        m_ui->cancel_button->show();
    }

    m_ui->ErrorList->setColumnWidth(0, 100);
    m_ui->ErrorList->clear();
    m_ui->details_button->setChecked(false);
    m_ui->details_button->setEnabled(false);

    show();
    resize(size().width(), sizeHint().height());
}

QHiMDUploadDialog::QHiMDUploadDialog(QWidget *parent, enum Mode mode)
    : QDialog(parent)
    , m_ui(new Ui::QHiMDUploadDialog)
    , canceled(false)
    , mode(mode)
{
    m_ui->setupUi(this);

    setWindowTitle((mode == UPLOAD) ? tr("Track upload") : tr("Track download"));

    if (mode == DOWNLOAD) {
        m_ui->icon->setPixmap(QPixmap(":/icons/download_to_md.png"));
    }
}

QHiMDUploadDialog::~QHiMDUploadDialog()
{
    delete m_ui;
}

void QHiMDUploadDialog::on_details_button_toggled(bool checked)
{
    if (checked)
    {
        m_ui->line->show();
        m_ui->ErrorList->show();
    }
    else
    {
        m_ui->line->hide();
        m_ui->ErrorList->hide();
    }
    /* Need to process events to make the show or hide calls take effect
       before calling sizeHint() */
    QApplication::processEvents();
    resize(size().width(), sizeHint().height());
}

void QHiMDUploadDialog::on_close_button_clicked()
{
    close();
}

void QHiMDUploadDialog::on_cancel_button_clicked()
{
    m_ui->alltrack_label->setText((mode == UPLOAD) ? tr("upload aborted by the user") : tr("download aborted by the user"));
    canceled = true;
}
