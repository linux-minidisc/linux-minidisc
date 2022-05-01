#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"
#include "qhimdaboutdialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>

void QHiMDMainWindow::set_buttons_enable()
{
    bool have_device = (current_device != nullptr);
    auto *model = ui->TrackList->selectionModel();
    bool minidisc_tracks_selected = model != nullptr && model->selectedRows(0).length() != 0;

    bool is_writable = have_device && current_device->isWritable();
    bool can_download = have_device && is_writable;
    bool can_upload = have_device && current_device->canUpload() && minidisc_tracks_selected;
    bool can_edit = have_device && minidisc_tracks_selected;
    bool can_format = have_device && is_writable && current_device->canFormatDisk();

    ui->action_Connect->setEnabled(!have_device);
    ui->action_Download->setEnabled(can_download);
    ui->action_Upload->setEnabled(can_upload);
    ui->action_Rename->setEnabled(can_edit);
    ui->action_Delete->setEnabled(can_edit);
    ui->action_Format->setEnabled(can_format);
    ui->action_Quit->setEnabled(true);
}

static QString
browser_for_device_type(device_type type)
{
    return type == NETMD_DEVICE ? "netmd_browser" : "himd_browser";
}

void QHiMDMainWindow::init_himd_browser(QMDTracksModel * model)
{
    int i, width;
    ui->TrackList->setModel(model);

    QObject::connect(ui->TrackList->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
                     this, SLOT(handle_himd_selection_change(const QItemSelection&, const QItemSelection&)));

    // read saved column width for this model
    QString browser = browser_for_device_type(current_device->deviceType());
    for(i = 0; i < ui->TrackList->model()->columnCount(); i++)
    {
        width = settings.value(browser + QString::number(i), 0).toInt();
        if(width != 0)
            ui->TrackList->setColumnWidth(i, width);
    }
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
            // XXX: Duplicate code with on_action_Connect_triggered()
            path = QFileDialog::getExistingDirectory(this,
                                                     tr("Select mountpath for %1").arg(dev->name()),
                                                     settings.value("lastImageDirectory", QDir::currentPath()).toString(),
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
        // TODO: make sure current_device is nullptr
        set_buttons_enable();
        ui->himd_devices->setCurrentIndex(0);
        return;
     }

    if (!current_device->isWritable()) {
        ui->statusBar->showMessage(tr("MiniDisc is write-protected, download and edit disabled"), 10000);
    }

    ui->DiscTitle->setText(current_device->discTitle());
    set_buttons_enable();
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
    current_device = NULL;
    detect = createDetection(this);
    ui->setupUi(this);
    set_buttons_enable();
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
    if (!current_device) {
        return;
    }

    QStringList DownloadFileList;

    QString extensions = "";
    for (const auto &extension: current_device->downloadableFileExtensions()) {
        if (!extensions.isEmpty()) {
            extensions += " ";
        }
        extensions += "*." + extension;
    }

    DownloadFileList = QFileDialog::getOpenFileNames(
                         this,
                         tr("Select files for download"),
                         QDir::currentPath(),
                         tr("Supported files") + " (" + extensions + ")");

    QApplication::processEvents();

    for (auto &fileName: DownloadFileList) {
        // TODO: Show UI progress while download is taking place (right now, UI freezes)
        if (current_device->download(fileName)) {
            // Open device again to refresh the UI
            open_device(current_device);
        } else {
            QMessageBox errormsg;
            errormsg.setText(tr("Downloading of files failed."));
            errormsg.exec();
        }
    }
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

void QHiMDMainWindow::on_action_Rename_triggered()
{
    QMessageBox::information(this, "Not implemented", "This feature is not implemented yet");
}

void QHiMDMainWindow::on_action_Delete_triggered()
{
    QMessageBox::information(this, "Not implemented", "This feature is not implemented yet");
}

void QHiMDMainWindow::on_action_Format_triggered()
{
    if (!current_device) {
        return;
    }

    if (QMessageBox::question(this,
                tr("Format medium"),
                tr("Really format MiniDisc (all tracks and data will be lost)?")) == QMessageBox::Yes) {
        if (!current_device->formatDisk()) {
            QMessageBox::information(this, tr("Operation failed"), tr("Could not format medium."));
        } else {
            // Open device again to refresh the UI
            open_device(current_device);
        }
    }
}

void QHiMDMainWindow::on_action_Connect_triggered()
{
    int index;
    QHiMDDevice *dev;
    QString HiMDDirectory;
    HiMDDirectory = settings.value("lastImageDirectory", QDir::currentPath()).toString();
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

void QHiMDMainWindow::handle_himd_selection_change(const QItemSelection&, const QItemSelection&)
{
    // Re-evaluate which buttons are enabled
    set_buttons_enable();
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

    QMDTracksModel * mod = (QMDTracksModel *)ui->TrackList->model();

    QObject::disconnect(current_device, SIGNAL(closed()), this, SLOT(current_device_closed()));

    // save column width for this model first
    QString browser = browser_for_device_type(current_device->deviceType());
    for(i = 0;i < mod->columnCount(); i++) {
        settings.setValue(browser + QString::number(i), ui->TrackList->columnWidth(i));
    }

    mod->close();
    current_device = NULL;
    ui->DiscTitle->setText(QString());
    ui->himd_devices->setCurrentIndex(0);
    set_buttons_enable();
}
