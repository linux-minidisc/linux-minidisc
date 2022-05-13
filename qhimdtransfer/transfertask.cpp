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

#include "transfertask.h"


TransferTask::TransferTask(int trackIndex, const QString &filename, qlonglong totalSize, std::function<void(TransferTask &)> task)
    : QObject()
    , QRunnable()
    , mTrackIndex(trackIndex)
    , mFilename(filename)
    , mTotalSize(totalSize)
    , mTask(task)
    , mCanceled(false)
    , mFinished(false)
    , mSuccess(false)
    , mErrorMessage()
{
    // QHiMDUploadDialog takes care of task lifetime/ownership
    QRunnable::setAutoDelete(false);
}

TransferTask::~TransferTask()
{
}

void
TransferTask::run()
{
    emit taskStarted(mFilename, mTotalSize);

    mTask(*this);

    if (!mFinished) {
        mSuccess = true;
        mErrorMessage = "";
        mFinished = true;
    }

    emit taskFinished(mSuccess, mErrorMessage);
}

int
TransferTask::trackIndex() const
{
    return mTrackIndex;
}

const QString &
TransferTask::filename() const
{
    return mFilename;
}

qlonglong
TransferTask::totalSize() const
{
    return mTotalSize;
}

void
TransferTask::cancel()
{
    mCanceled = true;
}

void
TransferTask::progress(float progress)
{
    emit taskProgress(progress);
}

void
TransferTask::finish(bool success, const QString &errorMessage)
{
    mSuccess = success;
    mErrorMessage = errorMessage;
    mFinished = true;
}

bool
TransferTask::wasCanceled() const
{
    return mCanceled;
}
