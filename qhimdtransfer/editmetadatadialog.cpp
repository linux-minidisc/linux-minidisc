#include "editmetadatadialog.h"
#include "ui_editmetadatadialog.h"


EditMetadataDialog::EditMetadataDialog(QWidget *parent, QMDTrack *track)
    : QDialog(parent)
    , ui(new Ui::EditMetadataDialog())
    , track(track)
{
    ui->setupUi(this);

    setWindowTitle(tr("Editing metadata of track %1").arg(track->tracknum() + 1));

    ui->editTitle->setText(track->title());
    ui->editArtist->setText(track->artist());
    ui->editAlbum->setText(track->album());

    ui->editTitle->selectAll();
}

EditMetadataDialog::~EditMetadataDialog()
{
}

void
EditMetadataDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

QString
EditMetadataDialog::getTitle()
{
    return ui->editTitle->text();
}

QString
EditMetadataDialog::getArtist()
{
    return ui->editArtist->text();
}

QString
EditMetadataDialog::getAlbum()
{
    return ui->editAlbum->text();
}

static void
setAndFocus(QLineEdit *edit, const QString &text)
{
    edit->setText(text);
    edit->setFocus();
    edit->selectAll();
}

void
EditMetadataDialog::reloadTitleText()
{
    setAndFocus(ui->editTitle, track->title());
}

void
EditMetadataDialog::reloadArtistText()
{
    setAndFocus(ui->editArtist, track->artist());
}

void
EditMetadataDialog::reloadAlbumText()
{
    setAndFocus(ui->editAlbum, track->album());
}
