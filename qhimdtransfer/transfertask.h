#pragma once

/*
 * transfertask: Abstraction for tasks running in the background
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/


#include <QObject>
#include <QRunnable>
#include <QString>

#include <functional>

class TransferTask : public QObject, public QRunnable {
    Q_OBJECT

    int mTrackIndex;
    QString mFilename;
    qlonglong mTotalSize;
    std::function<void(TransferTask &)> mTask;
    bool mCanceled;
    bool mFinished;

    bool mSuccess;
    QString mErrorMessage;

public:
    TransferTask(int trackIndex, const QString &filename, qlonglong totalSize, std::function<void(TransferTask &)> task);
    virtual ~TransferTask();

    virtual void run() override;

    int trackIndex() const;
    const QString &filename() const;
    qlonglong totalSize() const;
    void cancel();

    void progress(float progress);
    void finish(bool success, const QString &errorMessage);
    bool wasCanceled() const;

signals:
    void taskStarted(QString filename, qlonglong totalSize);
    void taskProgress(float progress);
    void taskFinished(bool success, QString errorMessage);
};
