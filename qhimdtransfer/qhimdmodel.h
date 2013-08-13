#ifndef QHIMDMODEL_H
#define QHIMDMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QTime>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtGui/QFileSystemModel>
#include "himd.h"

#include "sony_oma.h"

class QHiMDTracksModel;

class QHiMDTrack {
    struct himd * himd;
    unsigned int trknum;
    unsigned int trackslot;
    struct trackinfo ti;
public:
    QHiMDTrack(struct himd * himd, unsigned int trackindex);
    unsigned int tracknum() const;
    QString title() const;
    QString artist() const;
    QString album() const;
    QString codecname() const;
    QTime duration() const;
    QDateTime recdate() const;
    bool copyprotected() const;
    int blockcount() const;

    QString openMpegStream(struct himd_mp3stream * str) const;
    QString openNonMpegStream(struct himd_nonmp3stream * str) const;
    QByteArray makeEA3Header() const;
};

typedef QList<QHiMDTrack> QHiMDTrackList;

class QHiMDTracksModel : public QAbstractListModel {
    Q_OBJECT

    struct himd *himd;
public:
    QHiMDTracksModel() : himd(NULL) {}
    /* QAbstractListModel stuff */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex() ) const;
    virtual int columnCount(const QModelIndex & parent = QModelIndex() ) const;
    /* HiMD containter stuff */
    QString open(const QString & path);	/* returns null if OK, error message otherwise */
    bool is_open();
    void close();
    QHiMDTrack track(int trackidx) const;
    QHiMDTrackList tracks(const QModelIndexList & indices) const;
    QStringList downloadableFileExtensions() const;
};

class QHiMDFileSystemModel : public QFileSystemModel {
    Q_OBJECT

    QStringList selectableExtensions;
public:
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    void setSelectableExtensions(QStringList extensions);
};


#endif
