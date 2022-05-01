#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"
#include "qhimdaboutdialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>

void QHiMDMainWindow::set_buttons_enable(bool connect, bool download, bool upload, bool rename, bool del, bool format, bool quit)
{
    ui->action_Connect->setEnabled(connect);
    ui->action_Download->setEnabled(download);
    ui->action_Upload->setEnabled(upload);
    ui->action_Rename->setEnabled(rename);
    ui->action_Delete->setEnabled(del);
    ui->action_Format->setEnabled(format);
    ui->action_Quit->setEnabled(quit);
    ui->upload_button->setEnabled(upload);
    ui->download_button->setEnabled(download);
}

void QHiMDMainWindow::init_himd_browser(QMDTracksModel * model)
{
    int i, width;
    QString browser = current_device->deviceType() == NETMD_DEVICE ? "netmd_browser" : "himd_browser";
    ui->TrackList->setModel(model);

    QObject::connect(ui->TrackList->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
                     this, SLOT(handle_himd_selection_change(const QItemSelection&, const QItemSelection&)));

    // read saved column width for this model
    for(i = 0; i < ui->TrackList->model()->columnCount(); i++)
    {
        width = settings.value(browser + QString::number(i), 0).toInt();
        if(width != 0)
            ui->TrackList->setColumnWidth(i, width);
    }
}

void QHiMDMainWindow::init_local_browser()
{
    QStringList DownloadFileList;
    localmodel.setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    localmodel.setNameFilters(QStringList() << "*.mp3" << "*.wav" << "*.oma" << "*.aea");
    localmodel.setNameFilterDisables(false);
    localmodel.setReadOnly(false);
    localmodel.setRootPath("/");
    ui->localScan->setModel(&localmodel);
    QModelIndex curdir = localmodel.index(ui->updir->text());
    ui->localScan->expand(curdir);
    ui->localScan->setCurrentIndex(curdir);
    ui->localScan->scrollTo(curdir,QAbstractItemView::PositionAtTop);
    ui->localScan->hideColumn(2);
    ui->localScan->hideColumn(3);
    ui->localScan->setColumnWidth(0, 350);
    QObject::connect(ui->localScan->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                     this, SLOT(handle_local_selection_change(const QItemSelection&, const QItemSelection&)));
}

void QHiMDMainWindow::save_window_settings()
{
    settings.setValue("geometry", QMainWindow::saveGeometry());
    settings.setValue("windowState", QMainWindow::saveState());
}

void QHiMDMainWindow::read_window_settings()
{
    QMainWindow::restoreGeometry(settings.value("geometry").toByteArray());
    QMainWindow::restoreState(settings.value("windowState").toByteArray());
}

bool QHiMDMainWindow::autodetect_init()
{
    if(!QObject::connect(detect, SIGNAL(deviceListChanged(QMDDevicePtrList)), this, SLOT(device_list_changed(QMDDevicePtrList))))
        return false;

    if(!detect->start_hotplug())
        detect->scan_for_minidisc_devices();
    return true;
}

void QHiMDMainWindow::setCurrentDevice(QMDDevice *dev)
{
    current_device = dev;
    QObject::connect(current_device, SIGNAL(closed()), this, SLOT(current_device_closed()));

    if(current_device->deviceType() == NETMD_DEVICE)
        init_himd_browser(&ntmodel);

    else if(current_device->deviceType() == HIMD_DEVICE)
        init_himd_browser(&htmodel);
}

void QHiMDMainWindow::open_device(QMDDevice * dev)
{
    QMessageBox mdStatus;
    QString error;
    QMDTracksModel * mod;
    QString path;

    int index = ui->himd_devices->currentIndex();  // remember current index of devices combo box, will be resetted by current_device_closed() function

    if (!dev)
    {
        current_device_closed();
        ui->himd_devices->setCurrentIndex(0);
        return;
    }

    if(current_device)
    {
        current_device_closed();
        ui->himd_devices->setCurrentIndex(index);  // set correct device index in the combo box
    }

    /* handle disc images, use previously choosen image path if availlable else ask for path
     * user can use connect button to choose another imape path */
    if(dev->deviceType() == HIMD_DEVICE && dev->path().isEmpty() && dev->name().contains("disc image"))
    {
        ui->himd_devices->setCurrentIndex(0);
        on_action_Connect_triggered();
        return;
    }

    /* handle himd devices, search for mountpoint first */
    if(dev->deviceType() == HIMD_DEVICE && dev->path().isEmpty())
    {
        path = detect->mountpoint(dev);

        /* ask for mountpoint if autodetection fails */
        if(path.isEmpty())
        {
            path = QFileDialog::getExistingDirectory(this,
                                                     tr("Select mountpath for %1").arg(dev->name()),
                                                     settings.value("lastImageDirectory", QDir::rootPath()).toString(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
        }
        /* break if no or invalid path is choosen */
        if(path.isEmpty())
        {
            ui->himd_devices->setCurrentIndex(0);
            return;
        }
        /* set manually choosen path if mountpoint detection fails */
        if(dev->path().isEmpty())
            dev->setPath(path);
        ui->himd_devices->setItemText(index, ui->himd_devices->currentText().append( " at " + dev->path() ));
    }

    setCurrentDevice(dev);
    mod = (QMDTracksModel *)ui->TrackList->model();
    error = mod->open(dev);

    if (!error.isEmpty())
    {
        mdStatus.setText(tr("Error opening minidisc device (") + current_device->name() + "):\n" + error);
        mdStatus.exec();
        set_buttons_enable(1,0,0,0,0,0,1);
        ui->himd_devices->setCurrentIndex(0);
        return;
     }

    localmodel.setSelectableExtensions(current_device->downloadableFileExtensions());
    QModelIndex curdir = localmodel.index(ui->updir->text());
    ui->localScan->expand(curdir);
    ui->localScan->setCurrentIndex(curdir);
    ui->DiscTitle->setText(current_device->discTitle());
    set_buttons_enable(1,0,0,1,1,1,1);
}

void QHiMDMainWindow::upload_to(const QString & UploadDirectory)
{
    QMDTrackIndexList tlist;

    foreach(QModelIndex index, ui->TrackList->selectionModel()->selectedRows(0))
        tlist.append(index.row());

    current_device->batchUpload(tlist, UploadDirectory);
}

QHiMDMainWindow::QHiMDMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::QHiMDMainWindowClass)
{
    aboutDialog = new QHiMDAboutDialog;
    formatDialog = new QHiMDFormatDialog;
    current_device = NULL;
    detect = createDetection(this);
    ui->setupUi(this);
    ui->updir->setText(settings.value("lastUploadDirectory",
                                         QDir::homePath()).toString());
    set_buttons_enable(1,0,0,0,0,0,1);
    init_local_browser();
    read_window_settings();
    ui->himdpath->hide();   // not needed, replaced by combo box
    if(!autodetect_init())
        ui->statusBar->showMessage(" autodetection disabled", 10000);
}

QHiMDMainWindow::~QHiMDMainWindow()
{
    if(current_device && current_device->isOpen())
        current_device->close();

    save_window_settings();
    delete ui;
}

/* Slots for the actions */

void QHiMDMainWindow::on_action_Download_triggered()
{
    QStringList DownloadFileList;


    DownloadFileList = QFileDialog::getOpenFileNames(
                         this,
                         tr("Select MP3s for download"),
                         "/",
                         "MP3-files (*.mp3)");

}

void QHiMDMainWindow::on_action_Upload_triggered()
{
    QString UploadDirectory = settings.value("lastManualUploadDirectory", QDir::homePath()).toString();
    UploadDirectory = QFileDialog::getExistingDirectory(this,
                                                 tr("Select directory for Upload"),
                                                 UploadDirectory,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    if(UploadDirectory.isEmpty())
        return;

    settings.setValue("lastManualUploadDirectory", UploadDirectory);
    upload_to(UploadDirectory);
}

void QHiMDMainWindow::on_action_Quit_triggered()
{
    close();
}

void QHiMDMainWindow::on_action_About_triggered()
{
    aboutDialog->show();
}

void QHiMDMainWindow::on_action_Help_triggered()
{
    // TODO: Change this once changes are merged upstream
    QDesktopServices::openUrl(QUrl("https://github.com/thp/linux-minidisc"));
}

void QHiMDMainWindow::on_action_Format_triggered()
{
    formatDialog->show();
}

void QHiMDMainWindow::on_action_Connect_triggered()
{
    int index;
    QHiMDDevice *dev;
    QString HiMDDirectory;
    HiMDDirectory = settings.value("lastImageDirectory", QDir::rootPath()).toString();
    HiMDDirectory = QFileDialog::getExistingDirectory(this,
                                                 tr("Select directory of HiMD Medium"),
                                                 HiMDDirectory,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    if(HiMDDirectory.isEmpty())
        return;

    index = ui->himd_devices->findText("disc image", Qt::MatchContains);
    ui->himd_devices->setCurrentIndex(index);   // index of disk image device
    dev = (QHiMDDevice *)ui->himd_devices->itemData(index).value<void *>();
    dev->setPath(HiMDDirectory);
    ui->himd_devices->setItemText(index, QString((dev->name() + " at " + dev->path() )));

    open_device(dev);
}

void QHiMDMainWindow::on_upload_button_clicked()
{
    upload_to(ui->updir->text());
}

void QHiMDMainWindow::handle_himd_selection_change(const QItemSelection&, const QItemSelection&)
{
    bool nonempty = ui->TrackList->selectionModel()->selectedRows(0).length() != 0;

    ui->action_Upload->setEnabled(nonempty);
    ui->upload_button->setEnabled(nonempty);
}

void QHiMDMainWindow::handle_local_selection_change(const QItemSelection&, const QItemSelection&)
{
    QModelIndex index = ui->localScan->currentIndex();
    bool download_possible = false;

    if(localmodel.fileInfo(index).isDir())
    {
        ui->updir->setText(localmodel.filePath(index));
        settings.setValue("lastUploadDirectory", localmodel.filePath(index));
    }

    if(localmodel.fileInfo(index).isFile())
        download_possible = current_device && current_device->isOpen();

    ui->action_Download->setEnabled(download_possible);
    ui->download_button->setEnabled(download_possible);
}

void QHiMDMainWindow::device_list_changed(QMDDevicePtrList dplist)
{
    QString device;
    QStringList devices;
    QMDDevice * dev;

    /* close current device if it is removed from device list, just to be sure, should be handled by closed() signal */
    if(current_device && current_device->isOpen() && !dplist.contains(current_device))
        current_device_closed();

    ui->himd_devices->clear();
    // add dummy entry for <disconnected>
    ui->himd_devices->addItem("<disconnected>");

    foreach(dev, dplist)
    {
        device = dev->name();
        if(dev->deviceType() == HIMD_DEVICE && !dev->path().isEmpty())
            device.append(" at " + dev->path());
        ui->himd_devices->addItem(device, QVariant::fromValue((void *)dev));
        if(!dev->name().contains("disc image"))
            devices << dev->name();
    }

    if(current_device)
        ui->himd_devices->setCurrentIndex(dplist.indexOf(current_device) + 1);
    else
    {
        /* do not open autodetected devices, just inform the user in the status bar
         * netmd devices: TOC may not be loaded completely in early state of detection
         * himd devices: device may not be mounted at this time (on unix: udev didn't finish processing)
         * user should wait for TOC loading/ mounting himd devices before opening */
        if(dplist.count() > 1)
        {
            ui->himd_devices->setCurrentIndex(0);
            ui->statusBar->showMessage(tr("detected minidisc devices : %1").arg(devices.join(" , ")));
        }
        else
            ui->statusBar->clearMessage();
    }
}

void QHiMDMainWindow::on_himd_devices_activated(QString device)
{
    QMDDevice * dev;
    int index = ui->himd_devices->findText(device);

    if (index == 0)  // disconnected
    {
        current_device_closed();
        return;
    }

    dev = (QMDDevice *)ui->himd_devices->itemData(index).value<void *>();
    open_device(dev);
}

void QHiMDMainWindow::current_device_closed()
{
    int i;

    if(!current_device)
        return;

    QString browser = current_device->deviceType() == NETMD_DEVICE ? "netmd_browser" : "himd_browser";
    QMDTracksModel * mod = (QMDTracksModel *)ui->TrackList->model();

    QObject::disconnect(current_device, SIGNAL(closed()), this, SLOT(current_device_closed()));

    // save column width for this model first
    for(i = 0;i < mod->columnCount(); i++)
        settings.setValue(browser + QString::number(i), ui->TrackList->columnWidth(i));

    mod->close();
    current_device = NULL;
    ui->DiscTitle->setText(QString());
    ui->himd_devices->setCurrentIndex(0);
    set_buttons_enable(1,0,0,0,0,0,1);
}

void QHiMDMainWindow::on_download_button_clicked()
{
    if (!current_device) {
        return;
    }

    // TODO: Show UI progress while download is taking place (right now, UI freezes)
    if (current_device->download(localmodel.filePath(ui->localScan->currentIndex()))) {
        // Open device again to refresh the UI
        open_device(current_device);
    } else {
        QMessageBox errormsg;
        errormsg.setText(tr("Downloading of files failed."));
        errormsg.exec();
    }
}
