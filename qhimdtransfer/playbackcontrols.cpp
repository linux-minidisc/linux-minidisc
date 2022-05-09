#include "playbackcontrols.h"
#include "ui_playbackcontrols.h"

#include "libnetmd.h"


PlaybackControls::PlaybackControls(QWidget *parent, QMDDevice *dev, QMDTracksModel *model)
    : QDialog(parent)
    , ui(new Ui::PlaybackControls())
    , dev(dev)
    , model(model)
    , timer(this)
    , updating(false)
{
    ui->setupUi(this);

    updating = true;
    for (int i=0; i<dev->trackCount(); ++i) {
        QMDTrack *track = model->getTrack(i);
        ui->cbTrackList->addItem(QString("%1 %2").arg(track->tracknum() + 1, 2, 10, QLatin1Char('0')).arg(track->title()));
    }
    updating = false;

    updateUIElements();

    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(updateUIElements()));

    setWindowTitle(tr("%1 - Playback Controls").arg(dev->name()));

    timer.start(500);
}

PlaybackControls::~PlaybackControls()
{
}

void
PlaybackControls::updateUIElements()
{
    auto netmd = dynamic_cast<QNetMDDevice *>(dev);

    if (!netmd) {
        return;
    }

    updating = true;

    auto devh = netmd->deviceHandle();

    netmd_time time;
    netmd_track_index current_track = netmd_get_playback_position(devh, &time);

    if (current_track != NETMD_INVALID_TRACK) {
        QMDTrack *track = model->getTrack(current_track);

        ui->cbTrackList->setCurrentIndex(current_track);

        if (!ui->sliderPlaybackPosition->isSliderDown()) {
            int positionSeconds = netmd_time_to_seconds(&time);

            char *str = netmd_time_to_string(&time);
            ui->lblPlaybackPosition->setText(str);
            netmd_free_string(str);

            ui->lblRemainingTime->setText(QString("-") + util::formatDuration(QTime(0, 0).addSecs(track->durationSeconds() - positionSeconds)));

            ui->sliderPlaybackPosition->setMinimum(0);
            ui->sliderPlaybackPosition->setMaximum(track->durationSeconds());
            ui->sliderPlaybackPosition->setValue(positionSeconds);
        }
    }

    updating = false;
}

void
PlaybackControls::changeEvent(QEvent *e)
{
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void
PlaybackControls::onPlay()
{
    dev->play();
}

void
PlaybackControls::onPause()
{
    dev->pause();
}

void
PlaybackControls::onStop()
{
    dev->stop();
}

void
PlaybackControls::onRewind()
{
    dev->startRewind();
}

void
PlaybackControls::onFastForward()
{
    dev->startFastForward();
}


void
PlaybackControls::onPreviousTrack()
{
    dev->gotoPreviousTrack();
}

void
PlaybackControls::onNextTrack()
{
    dev->gotoNextTrack();
}

void
PlaybackControls::onSeekToSliderPosition()
{
    int value = ui->sliderPlaybackPosition->value();

    auto netmd = dynamic_cast<QNetMDDevice *>(dev);

    if (!netmd) {
        return;
    }

    auto devh = netmd->deviceHandle();

    netmd_time time;
    netmd_track_index current_track = netmd_get_playback_position(devh, &time);

    time.frame = 0;
    time.second = value % 60;
    value /= 60;
    time.minute = value % 60;
    value /= 60;
    time.hour = value;

    netmd_set_playback_position(devh, current_track, &time);
}

void
PlaybackControls::onTrackActivated(int index)
{
    if (updating) {
        return;
    }

    auto netmd = dynamic_cast<QNetMDDevice *>(dev);

    if (!netmd) {
        return;
    }

    auto devh = netmd->deviceHandle();

    netmd_time time;
    netmd_track_index current_track = netmd_get_playback_position(devh, &time);

    if (index != current_track) {
        time.frame = 0;
        time.second = 0;
        time.minute = 0;
        time.hour = 0;

        netmd_set_playback_position(devh, index, &time);
    }
}
