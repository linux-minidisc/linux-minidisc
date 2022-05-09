#pragma once

#include <QDialog>
#include <QTimer>

#include "qmddevice.h"
#include "qmdmodel.h"

#include <memory>

namespace Ui {
    class PlaybackControls;
}

class PlaybackControls : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(PlaybackControls)

public:
    explicit PlaybackControls(QWidget *parent, QMDDevice *dev, QMDTracksModel *model);
    virtual ~PlaybackControls();

public slots:
    void onPlay();
    void onPause();
    void onStop();

    void onRewind();
    void onFastForward();

    void onPreviousTrack();
    void onNextTrack();

    void onSeekToSliderPosition();

    void updateUIElements();
    void onTrackActivated(int index);

protected:
    virtual void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui::PlaybackControls> ui;
    QMDDevice *dev;
    QMDTracksModel *model;
    QTimer timer;
    bool updating;
};
