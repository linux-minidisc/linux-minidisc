#include "qmdtrack.h"

static QString get_himd_str(struct himd * himd, int idx)
{
    QString outstr;
    char * str;
    if(!idx)
        return QString();
    str = himd_get_string_utf8(himd, idx, NULL, NULL);
    if(!str)
        return QString();

    outstr = QString::fromUtf8(str);
    himd_free(str);
    return outstr;
}

QHiMDTrack::QHiMDTrack(struct himd * himd, unsigned int trackindex) : himd(himd), trknum(trackindex)
{
    trackslot = himd_get_trackslot(himd, trackindex, NULL);
    if(trackslot != 0)
        if(himd_get_track_info(himd, trackslot, &ti, NULL) < 0)
            trackslot = -1;
}

QHiMDTrack::~QHiMDTrack()
{
    himd = NULL;
}

unsigned int QHiMDTrack::tracknum() const
{
    return trknum;
}

QString QHiMDTrack::title() const
{
    if(trackslot != 0)
        return get_himd_str(himd, ti.title);
    else
        return QString();
}

QString QHiMDTrack::artist() const
{
    if(trackslot != 0)
        return get_himd_str(himd, ti.artist);
    else
        return QString();
}

QString QHiMDTrack::album() const
{
    if(trackslot != 0)
        return get_himd_str(himd, ti.album);
    else
        return QString();
}

QString QHiMDTrack::codecname() const
{
    if(trackslot != 0)
        return himd_get_codec_name(&ti);
    else
        return QString();
}

QTime QHiMDTrack::duration() const
{
    QTime t(0,0,0);
    if(trackslot != 0)
        return t.addSecs(ti.seconds);
    else
        return QTime();
}

QDateTime QHiMDTrack::recdate() const
{
    QDate d(0,0,0);
    QTime t(0,0,0);
    if (trackslot != 0)
    {
        t.setHMS(ti.recordingtime.tm_hour,
          ti.recordingtime.tm_min,
          ti.recordingtime.tm_sec);
        d.setDate(ti.recordingtime.tm_year+1900,
          ti.recordingtime.tm_mon+1,
          ti.recordingtime.tm_mday);
        return QDateTime(d,t);
    }
    return QDateTime();
}

bool QHiMDTrack::copyprotected() const
{
    if(trackslot != 0)
        return !himd_track_uploadable(himd, &ti);
    return true;
}

int QHiMDTrack::blockcount() const
{
    if(trackslot != 0)
        return himd_track_blocks(himd, &ti, NULL);
    else
        return 0;
}

QString QHiMDTrack::openMpegStream(struct himd_mp3stream * str) const
{
    struct himderrinfo status;
    if(himd_mp3stream_open(himd, trackslot, str, &status) < 0)
        return QString::fromUtf8(status.statusmsg);
    return QString();
}

QString QHiMDTrack::openNonMpegStream(struct himd_nonmp3stream * str) const
{
    struct himderrinfo status;
    if(himd_nonmp3stream_open(himd, trackslot, str, &status) < 0)
        return QString::fromUtf8(status.statusmsg);
    return QString();
}

QByteArray QHiMDTrack::makeEA3Header() const
{
    char header[EA3_FORMAT_HEADER_SIZE];
    make_ea3_format_header(header, &ti.codec_info);
    return QByteArray(header,EA3_FORMAT_HEADER_SIZE);
}


QNetMDTrack::QNetMDTrack(netmd_dev_handle * deviceh, minidisc * my_md, int trackindex)
{
    devh = deviceh;
    md = my_md;
    trkindex = trackindex;

    netmd_error res = netmd_get_track_info(deviceh, trackindex, &info);

    if (res == NETMD_TRACK_DOES_NOT_EXIST) {
        trkindex = -1;
        return;  // no track with this trackindex
    } else if (res != NETMD_NO_ERROR) {
        trkindex = -1;
        return;  // error retrieving track information
    }

    int group = netmd_minidisc_get_track_group(md, trkindex);
    if (group != 0) {
        groupstring = QString(netmd_minidisc_get_group_name(md, group));
    }

    titlestring = QString(info.title);
    codecstring = QString(netmd_get_encoding_name(info.encoding));
}

QNetMDTrack::~QNetMDTrack()
{
    devh = NULL;
    md = NULL;
}

unsigned int QNetMDTrack::tracknum() const
{
    /* returns zero based track number, maybe this function should return a one based track number as shown in the treeview,
     * trackindex -> zero based; tracknumber -> one based
     */
    return trkindex;
}

QString QNetMDTrack::group() const
{
    if(trkindex < 0)
        return QString();

    return groupstring;
}

QString QNetMDTrack::title() const
{
    if(trkindex < 0)
        return QString();

    return titlestring;
}

QString QNetMDTrack::codecname() const
{
    if(trkindex < 0)
        return QString();

    return codecstring;
}

QTime QNetMDTrack::duration() const
{
    QTime t(0,0,0);

    if(trkindex < 0)
        return QTime();

    return t.addSecs(info.duration.minute * 60 + info.duration.second);
}

bool QNetMDTrack::copyprotected() const
{
    return info.protection != NETMD_TRACK_FLAG_UNPROTECTED;
}

int QNetMDTrack::blockcount() const
{
    // Dummy "blocks" value, the value is just used to "weight" the
    // relative track durations in the QHiMDUploadDialog currently.
    return 100 * (info.duration.minute * 60 + info.duration.second);
}
