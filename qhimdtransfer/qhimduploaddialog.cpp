#include "qhimduploaddialog.h"
#include "ui_qhimduploaddialog.h"

#include <QFileInfo>
#include <QThreadPool>
#include <QCloseEvent>


void
QHiMDUploadDialog::addTask(int trackIndex, const QString &filename, qlonglong totalSize, std::function<void(TransferTask &)> task)
{
    // TODO: Use std::make_unique later
    tasks.emplace_back(new TransferTask(trackIndex, filename, totalSize, task));
}

int
QHiMDUploadDialog::exec()
{
    canceled = false;
    ready = false;

    currentTask = 1;
    tasksCount = tasks.size();

    elapsedTimer.start();
    updateTimerID = startTimer(100);
    estimatedTotalDuration = -1;

    allTasksSize = 0;
    for (auto &task: tasks) {
        allTasksSize += task->totalSize();
    }
    finishedTasksSize = 0;

    m_ui->AllPBar->setRange(0, 0);
    m_ui->AllPBar->reset();

    if (tasksCount == 1) {
        m_ui->AllPBar->hide();
    } else {
        m_ui->AllPBar->show();
    }

    m_ui->TrkPBar->setRange(0, 0);
    m_ui->TrkPBar->reset();

    m_ui->OverallProgress->setText("");

    tasksSucceeded = 0;
    tasksFailed = 0;
    tasksCanceled = 0;

    m_ui->curtrack_label->setText("");
    m_ui->curtrack_label->show();
    m_ui->current->setMinimumSize(0,0);
    m_ui->close_button->hide();
    m_ui->cancel_button->setEnabled(mode != REFRESH);
    m_ui->cancel_button->show();

    m_ui->ErrorList->setColumnWidth(0, 100);
    m_ui->ErrorList->clear();
    m_ui->details_button->setChecked(false);
    m_ui->details_button->setEnabled(false);

    // Kick off task processing
    runNextTask();

    return QDialog::exec();
}

void
QHiMDUploadDialog::runNextTask()
{
    if (canceled) {
        m_ui->OverallProgress->setText(tr("Operation canceled by user"));

        tasksCanceled += tasks.size();
        tasks.clear();
    }

    {
        QStringList messages;

        if (tasksSucceeded != 0) {
            messages << QString("%1 succeeded").arg(tasksSucceeded);
        }

        if (tasksFailed != 0) {
            messages << QString("%1 failed").arg(tasksFailed);
        }

        if (tasksCanceled != 0) {
            messages << QString("%1 canceled").arg(tasksCanceled);
        }

        m_ui->Summary->setText(messages.join(", "));
    }

    if (tasks.empty()) {
        killTimer(updateTimerID);
        m_ui->ETA->setText(tr("Operation completed in %1").arg(QTime(0, 0).addMSecs(elapsedTimer.elapsed()).toString("mm:ss")));

        m_ui->TrkPBar->setMaximum(1);
        m_ui->TrkPBar->setValue(1);
        m_ui->AllPBar->setMaximum(1);
        m_ui->AllPBar->setValue(1);
        m_ui->curtrack_label->setText("");

        m_ui->cancel_button->hide();
        m_ui->close_button->show();

        // Free all the TransferTask objects that have finished
        finishedTasks.clear();

        ready = true;

        if (mode == REFRESH) {
            // Finish the dialog immediately
            accept();
        }

        return;
    }

    std::unique_ptr<TransferTask> task = std::move(tasks.front());
    tasks.pop_front();

    QObject::connect(task.get(), &TransferTask::taskStarted,
                     this, &QHiMDUploadDialog::onTaskStarted);

    QObject::connect(task.get(), &TransferTask::taskProgress,
                     this, &QHiMDUploadDialog::onTaskProgress);

    QObject::connect(task.get(), &TransferTask::taskFinished,
                     this, &QHiMDUploadDialog::onTaskFinished);

    {
        QMutexLocker lock(&currentTransferTaskMutex);
        currentTransferTask = std::move(task);
        QThreadPool *pool = QThreadPool::globalInstance();
        pool->start(currentTransferTask.get());
    }
}

void
QHiMDUploadDialog::onTaskStarted(QString filename, qlonglong totalSize)
{
    switch (mode) {
        case UPLOAD:
            m_ui->OverallProgress->setText(tr("Uploading"));
            break;
        case DOWNLOAD:
            m_ui->OverallProgress->setText(tr("Downloading"));
            break;
        case REFRESH:
            m_ui->OverallProgress->setText(tr("Refreshing"));
            break;
    }

    if (tasksCount != 1) {
        m_ui->OverallProgress->setText(m_ui->OverallProgress->text() +
                QString(" %1 / %2").arg(currentTask).arg(tasksCount));
    }

    currentTask++;

    currentTaskSize = totalSize;

    m_ui->TrkPBar->setRange(0, 0);
    m_ui->TrkPBar->reset();
    m_ui->TrkPBar->show();

    // No estimate during transfer startup
    estimatedTotalDuration = -1;

    show();
    resize(size().width(), sizeHint().height());

    currentFilename = filename;
    m_ui->curtrack_label->setText(QFileInfo(filename).fileName());
}

void
QHiMDUploadDialog::onTaskProgress(float progress)
{
    m_ui->TrkPBar->setRange(0, 100);
    m_ui->TrkPBar->setValue(progress * 100);

    m_ui->AllPBar->setRange(0, allTasksSize * 100);
    m_ui->AllPBar->setValue((finishedTasksSize + float(currentTaskSize) * progress) * 100);

    if (m_ui->AllPBar->maximum() != 0) {
        float fraction = (float)(m_ui->AllPBar->value() - m_ui->AllPBar->minimum()) /
                         (float)(m_ui->AllPBar->maximum() - m_ui->AllPBar->minimum());

        if (fraction != 0.f && progress != 1.f) {
            qint64 duration = elapsedTimer.elapsed();
            estimatedTotalDuration = duration / fraction;
        }
    }
}

void
QHiMDUploadDialog::onTaskFinished(bool success, QString errorMessage)
{
    {
        QMutexLocker lock(&currentTransferTaskMutex);
        // Clear the current transfer task for later deletion
        finishedTasks.push_back(std::move(currentTransferTask));
    }

    finishedTasksSize += currentTaskSize;
    m_ui->AllPBar->setValue(finishedTasksSize * 100);

    if (success) {
        tasksSucceeded++;
    } else {
        tasksFailed++;

        QTreeWidgetItem * ErrorMsg = new QTreeWidgetItem();
        ErrorMsg->setText(0, currentFilename);
        ErrorMsg->setText(1, errorMessage);
        m_ui->ErrorList->insertTopLevelItem(0, ErrorMsg);
        m_ui->details_button->setEnabled(true);
    }

    runNextTask();
}

QHiMDUploadDialog::QHiMDUploadDialog(QWidget *parent, enum Mode mode)
    : QDialog(parent)
    , m_ui(new Ui::QHiMDUploadDialog)
    , canceled(false)
    , mode(mode)
    , currentTransferTaskMutex()
    , currentTransferTask(nullptr)
    , updateTimerID(-1)
{
    m_ui->setupUi(this);

    switch (mode) {
        case UPLOAD:
            setWindowTitle(tr("Track upload"));
            break;
        case DOWNLOAD:
            setWindowTitle(tr("Track download"));
            break;
        case REFRESH:
            setWindowTitle(tr("Getting track list"));
            break;
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
    accept();
}

void QHiMDUploadDialog::on_cancel_button_clicked()
{
    m_ui->OverallProgress->setText(tr("Canceling, please wait"));

    {
        QMutexLocker lock(&currentTransferTaskMutex);

        if (currentTransferTask != nullptr) {
            currentTransferTask->cancel();
        }
    }

    canceled = true;
}

void
QHiMDUploadDialog::timerEvent(QTimerEvent *event)
{
    qint64 duration = elapsedTimer.elapsed();
    m_ui->ETA->setText(tr("Elapsed: %1").arg(QTime(0, 0).addMSecs(duration).toString("mm:ss")));

    // Only start showing estimates after 5 seconds
    // FIXME: This estimate is still kind of bad and jumpy, because there's
    // some spin-up time and other time spent in the MD recorder that's not
    // reflected in the percentage of "just" the core audio transfer
    // (at least for NetMD, Hi-MD estimates are quite good already)
    if (estimatedTotalDuration != -1 && duration > 5000) {
        qint64 estimatedRemainingTime = estimatedTotalDuration - duration;

        if (estimatedRemainingTime < 0) {
            return;
        }

        m_ui->ETA->setText(m_ui->ETA->text() + ", " + tr("ETA: %1").arg(QTime(0, 0).addMSecs(estimatedRemainingTime).toString("mm:ss")));
    }
}

void
QHiMDUploadDialog::closeEvent(QCloseEvent *event)
{
    if (!canceled) {
        on_cancel_button_clicked();
        event->ignore();
    } else if (!ready) {
        event->ignore();
    } else {
        QDialog::closeEvent(event);
    }
}

void
QHiMDUploadDialog::runSingleTask(QWidget *parent, enum Mode mode, const QString &title, std::function<void(TransferTask &)> task)
{
    QHiMDUploadDialog dialog(parent, mode);
    dialog.addTask(-1, title, 1, task);
    dialog.exec();
}
