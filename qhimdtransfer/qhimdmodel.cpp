#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include "qhimdmodel.h"

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
	QDate d;
	QTime t;
	QDateTime dt;
	if (trackslot != 0) {
		t.setHMS(ti.recordingtime.tm_hour,
			ti.recordingtime.tm_min,
			ti.recordingtime.tm_sec);
		d.setDate(ti.recordingtime.tm_year+1900,
			ti.recordingtime.tm_mon+1,
			ti.recordingtime.tm_mday);
		dt.setDate(d);
		dt.setTime(t);
	} else
		return dt;
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

enum columnum {
  ColId, ColTitle, ColArtist, ColAlbum, ColLength, ColCodec, ColUploadable, ColRecDate,
  LAST_columnnum = ColRecDate
};

QVariant QHiMDTracksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal)
        return QVariant();

    if(role == Qt::SizeHintRole)
    {
        static QFont f;
        static QFontMetrics met(f);
        switch((columnum)section)
        {
            case ColId:
                return QSize(met.width("9999")+5, 0);
            case ColTitle:
            case ColArtist:
            case ColAlbum:
                return QSize(25*met.averageCharWidth(), 0);
            case ColLength:
                return QSize(met.width("9:99:99"), 0);
            case ColCodec:
            case ColUploadable:
                /* Really use the header for the metric in these columns,
                   contents will be shorter */
                return QAbstractListModel::headerData(section,orientation,role);
			case ColRecDate:
				return QSize(met.width("yyyy.MM.dd hh:mm:ss"), 0);
        }
    }

    if(role == Qt::DisplayRole)
    {
        switch((columnum)section)
        {
            case ColId:
                return tr("Nr.");
            case ColTitle:
                return tr("Title");
            case ColArtist:
                return tr("Artist");
            case ColAlbum:
                return tr("Album");
            case ColLength:
                return tr("Length");
            case ColCodec:
                return tr("Format");
            case ColUploadable:
                return tr("Uploadable");
            case ColRecDate:
                return tr("Recorded At");
        }
    }
    return QVariant();
}

QVariant QHiMDTracksModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::TextAlignmentRole && 
       (index.column() == ColId || index.column() == ColLength))
        return Qt::AlignRight;

    if(index.row() >= rowCount())
        return QVariant();

    QHiMDTrack track(himd, index.row());

    if(role == Qt::CheckStateRole && index.column() == ColUploadable)
        return track.copyprotected() ? Qt::Unchecked : Qt::Checked;

    if(role == Qt::DisplayRole)
    {
        switch((columnum)index.column())
        {
            case ColId:
                return index.row() + 1;
            case ColTitle:
                return track.title();
            case ColArtist:
                return track.artist();
            case ColAlbum:
                return track.album();
            case ColLength:
            {
                QTime t = track.duration();
                if(t < QTime(1,0,0))
                    return t.toString("m:ss");
                else
                    return t.toString("h:mm:ss");
            }
            case ColCodec:
                return track.codecname();
            case ColUploadable:
                return QVariant(); /* Displayed by checkbox */
			case ColRecDate:
			{
				QDateTime dt = track.recdate();
				return dt.toString("yyyy.MM.dd hh:mm:ss");
			}
        }
    }
    return QVariant();
}

int QHiMDTracksModel::rowCount(const QModelIndex &) const
{
    if(himd)
        return himd_track_count(himd);
    else
        return 0;
}

int QHiMDTracksModel::columnCount(const QModelIndex &) const
{
    return LAST_columnnum+1;
}

QString QHiMDTracksModel::open(const QString & path)
{
    struct himd * newhimd;
    struct himderrinfo status;

    newhimd = new struct himd;
    if(himd_open(newhimd,path.toUtf8(), &status) < 0)
    {
        delete newhimd;
        return QString::fromUtf8(status.statusmsg);
    }
    close();
    beginResetModel();
    himd = newhimd;
    endResetModel();	/* inform views that the model contents changed */
    return QString();
}

bool QHiMDTracksModel::is_open()
{
    return himd != NULL;
}

void QHiMDTracksModel::close()
{
    struct himd * oldhimd;
    if(!himd)
        return;
    beginResetModel();
    oldhimd = himd;
    himd = NULL;
    endResetModel();	/* inform views that the model contents changed */
    himd_close(oldhimd);
    delete oldhimd;
}

QHiMDTrack QHiMDTracksModel::track(int trknum) const
{
    return QHiMDTrack(himd, trknum);
}

QHiMDTrackList QHiMDTracksModel::tracks(const QModelIndexList & modelindices) const
{
    QHiMDTrackList tracks;
    QModelIndex index;
    foreach(index, modelindices)
        tracks.append(track(index.row()));
    return tracks;
}

QStringList QHiMDTracksModel::downloadableFileExtensions() const
{
    return (QStringList() << "mp3");
}

/* QFileSystemModel stuff */

Qt::ItemFlags QHiMDFileSystemModel::flags(const QModelIndex &index) const
{
    if(!isDir(index) && !selectableExtensions.contains((fileInfo(index).suffix()), Qt::CaseInsensitive))
        return Qt::NoItemFlags;   //not selectable, not enabled (grayed out in the browser)

    return QFileSystemModel::flags(index);
}

void QHiMDFileSystemModel::setSelectableExtensions(QStringList extensions)
{
    selectableExtensions = extensions;
}
