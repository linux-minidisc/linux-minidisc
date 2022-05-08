#ifndef QMDTRACK_H
#define QMDTRACK_H

#include <QTime>
#include "himd.h"
#include "sony_oma.h"

#ifdef Q_OS_WIN
    #ifdef WINVER            // WINVER needs to be 0x500 or later to make the windows autodetection mechanism work and it
        #undef WINVER        // must be defined correctly before including libusb.h (included from libnetmd.h), else it will be defined
    #endif                   // in windef.h to 0x400
    #define WINVER 0x500
#endif

#include "libnetmd.h"

/* define buffer size for netmd uploads */
#define NETMD_RECV_BUF_SIZE 0x10000

class QMDTrack
{
public:
    QMDTrack() {}   // returns dummy data, implemented to have a common class name with common members
    virtual ~QMDTrack() {}
    virtual unsigned int tracknum() const {return -1;}
    virtual QString group() const {return QString();}
    virtual QString title() const {return QString();}
    virtual QString artist() const {return QString();}
    virtual QString album() const {return QString();}
    virtual QString codecname() const {return QString();}
    virtual QTime duration() const {return QTime();}
    virtual bool copyprotected() const {return true;}
    virtual int blockcount() const {return 0;}
};

class QHiMDTrack : public QMDTrack{
    struct himd * himd;
    unsigned int trknum;
    unsigned int trackslot;
    struct trackinfo ti;
public:
    QHiMDTrack(struct himd * himd, unsigned int trackindex);
    virtual ~QHiMDTrack();
    virtual unsigned int tracknum() const;
    virtual QString title() const;
    virtual QString artist() const;
    virtual QString album() const;
    virtual QString codecname() const;
    virtual QTime duration() const;
    QDateTime recdate() const;
    virtual bool copyprotected() const;
    virtual int blockcount() const;

    QString openMpegStream(struct himd_mp3stream * str) const;
    QString openNonMpegStream(struct himd_nonmp3stream * str) const;
    QByteArray makeEA3Header() const;

    bool updateMetadata(const QString &title, const QString &artist, const QString &album);
};

class QNetMDTrack : public QMDTrack {
    netmd_dev_handle * devh;
    int8_t trkindex;
    struct netmd_track_info info;
    QString groupstring;
    QString titlestring;
    QString codecstring;
public:
    QNetMDTrack(netmd_dev_handle *deviceh, int trackindex);
    virtual ~QNetMDTrack();
    virtual unsigned int tracknum() const;
    virtual QString group() const;
    virtual QString title() const;
    virtual QString codecname() const;
    virtual QTime duration() const;
    virtual bool copyprotected() const;
    virtual int blockcount() const;

    bool rename(const QString &);
};

typedef QList<QMDTrack> QMDTrackList;
typedef QList<QHiMDTrack> QHiMDTrackList;
typedef QList<QNetMDTrack> QNetMDTrackList;
typedef QList<unsigned int> QMDTrackIndexList;

#endif // QMDTRACK_H
