#ifndef QMDMODEL_H
#define QMDMODEL_H

#include <QtCore/QAbstractListModel>

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QFileSystemModel>
#include <qmddevice.h>

class QMDTracksModel : public QAbstractListModel {
    Q_OBJECT

    QMDDevice * dev;
public:
    QMDTracksModel() : dev(NULL) {}
    /* QAbstractListModel stuff */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {return QVariant();}
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const {return QVariant();}
    virtual int rowCount(const QModelIndex & parent = QModelIndex() ) const {return 0;}
    virtual int columnCount(const QModelIndex & parent = QModelIndex() ) const {return 0;}
    /* dummy data for unknown devices */
    virtual QString open(QMDDevice *device = NULL) {return tr("no known device type specified");}
    virtual bool is_open() {return false;}
    virtual void close() {}
    QStringList downloadableFileExtensions() const {return QStringList();}
};

class QNetMDTracksModel : public QMDTracksModel {
    Q_OBJECT

    QNetMDDevice * ndev;
    QNetMDTrackList allTracks;
public:
    QNetMDTracksModel() {ndev = NULL;}
    /* QAbstractListModel stuff */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex() ) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex() ) const;
    /* NetMD device stuff */
    QString open(QMDDevice *device);	/* returns null if OK, error message otherwise */
    virtual bool is_open();
    void close();
    QNetMDTrack track(int trkidx) const;
    virtual QNetMDTrackList tracks(const QModelIndexList & indices) const;  // should be QMDTrackList later
    QStringList downloadableFileExtensions() const;
};

class QHiMDTracksModel : public QMDTracksModel {
    Q_OBJECT

    QHiMDDevice * hdev;
public:
    QHiMDTracksModel() {hdev = NULL;}
    /* QAbstractListModel stuff */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex() ) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex() ) const;
    /* HiMD containter stuff */
    virtual QString open(QMDDevice *device);	/* returns null if OK, error message otherwise */
    virtual bool is_open();
    virtual void close();
    virtual QHiMDTrack track(int trackidx) const;
    virtual QHiMDTrackList tracks(const QModelIndexList & indices) const;  // should be QMDTrackList later
    QStringList downloadableFileExtensions() const;
};

class QHiMDFileSystemModel : public QFileSystemModel {
    Q_OBJECT

    QStringList selectableExtensions;
public:
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    void setSelectableExtensions(QStringList extensions);
};

#endif // QMDMODEL_H
