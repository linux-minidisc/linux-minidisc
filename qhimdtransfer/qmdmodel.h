#ifndef QMDMODEL_H
#define QMDMODEL_H

#include <QAbstractListModel>

#include <QList>
#include <QStringList>
#include <qmddevice.h>

class QMDTracksModel : public QAbstractListModel {
    Q_OBJECT

public:
    QMDTracksModel() {}
    /* Make this method from QAbstractListModel public */
    virtual int columnCount(const QModelIndex & parent = QModelIndex() ) const = 0;
    /* dummy data for unknown devices */
    virtual QString open(QMDDevice *device = NULL) = 0;
    virtual bool is_open() {return false;}
    virtual void close() {}
    virtual QMDTrack *getTrack(int index) = 0;
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
    virtual QMDTrack *getTrack(int index);
};

class QHiMDTracksModel : public QMDTracksModel {
    Q_OBJECT

    QHiMDDevice * hdev;
    QHiMDTrackList allTracks;
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
    virtual QMDTrack *getTrack(int index);
};

#endif // QMDMODEL_H
