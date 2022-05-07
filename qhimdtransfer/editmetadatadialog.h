#pragma once

#include <QDialog>

#include "qmdtrack.h"

#include <memory>

namespace Ui {
    class EditMetadataDialog;
}

class EditMetadataDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(EditMetadataDialog)

public:
    explicit EditMetadataDialog(QWidget *parent, QMDTrack *track);
    virtual ~EditMetadataDialog();

    QString getTitle();
    QString getArtist();
    QString getAlbum();

public slots:
    void reloadTitleText();
    void reloadArtistText();
    void reloadAlbumText();

protected:
    virtual void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui::EditMetadataDialog> ui;
    QMDTrack *track;
};
