#ifndef QHIMDUPLOADDIALOG_H
#define QHIMDUPLOADDIALOG_H

#include <QDialog>
#include <QElapsedTimer>
#include <QMutex>

#include <deque>
#include <memory>

#include "qmdtrack.h"
#include "transfertask.h"

namespace Ui {
    class QHiMDUploadDialog;
}

class QHiMDUploadDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode {
        UPLOAD = 0,
        DOWNLOAD = 1,
        REFRESH = 2,
    };

    explicit QHiMDUploadDialog(QWidget *parent, enum Mode mode);
    virtual ~QHiMDUploadDialog();
    bool was_canceled() { return canceled; }

    void addTask(int trackIndex, const QString &filename, qlonglong totalSize, std::function<void(TransferTask &)> task);

    virtual int exec();

    static void runSingleTask(QWidget *parent, enum Mode mode, const QString &title, std::function<void(TransferTask &)> task);

public slots:
    void onTaskStarted(QString filename, qlonglong totalSize);
    void onTaskProgress(float progress);
    void onTaskFinished(bool success, QString errorMessage);

protected:
    virtual void timerEvent(QTimerEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;

private:
    void runNextTask();

    Ui::QHiMDUploadDialog *m_ui;

    int tasksSucceeded, tasksFailed, tasksCanceled;

    bool canceled;
    bool ready;
    enum Mode mode;
    QString currentFilename;

    int tasksCount;
    int currentTask;
    qlonglong allTasksSize;
    qlonglong finishedTasksSize;
    qlonglong currentTaskSize;
    std::deque<std::unique_ptr<TransferTask>> tasks;
    std::vector<std::unique_ptr<TransferTask>> finishedTasks;

    QMutex currentTransferTaskMutex;
    std::unique_ptr<TransferTask> currentTransferTask;

    QElapsedTimer elapsedTimer;
    int updateTimerID;
    qint64 estimatedTotalDuration;

private slots:
    /* UI slots */
    void on_close_button_clicked();
    void on_cancel_button_clicked();
    void on_details_button_toggled(bool checked);
};

#endif // QHIMDUPLOADDIALOG_H
