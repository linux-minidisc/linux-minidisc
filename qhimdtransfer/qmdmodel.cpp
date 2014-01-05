#include <QFont>
#include <QFontMetrics>
#include <qmdmodel.h>

enum hcolumnum {
  ColId, ColTitle, ColArtist, ColAlbum, ColLength, ColCodec, ColUploadable, ColRecDate,
  LAST_hcolumnnum = ColRecDate
};

enum ncolumnum {
  CoId, CoGroup, CoTitle, CoLength, CoCodec, CoUploadable,
  LAST_ncolumnnum = CoUploadable
};


/* netmd tracks model */
QVariant QNetMDTracksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal)
        return QVariant();

    if(role == Qt::SizeHintRole)
    {
        static QFont f;
        static QFontMetrics met(f);
        switch((ncolumnum)section)
        {
            case CoId:
                return QSize(met.width("9999")+5, 0);
            case CoGroup:
            case CoTitle:
            case CoLength:
                return QSize(met.width("9:99:99"), 0);
            case CoCodec:
            case CoUploadable:
                /* Really use the header for the metric in these columns,
                   contents will be shorter */
                return QAbstractListModel::headerData(section,orientation,role);
        }
    }

    if(role == Qt::DisplayRole)
    {
        switch((ncolumnum)section)
        {
            case CoId:
                return tr("Nr.");
            case CoGroup:
                return tr("Group");
            case CoTitle:
                return tr("Title");
            case CoLength:
                return tr("Length");
            case CoCodec:
                return tr("Format");
            case CoUploadable:
                return tr("Uploadable");
        }
    }
    return QVariant();
}

QVariant QNetMDTracksModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::TextAlignmentRole &&
       (index.column() == CoId || index.column() == CoLength))
        return Qt::AlignRight;

    if(index.row() >= rowCount())
        return QVariant();

    QNetMDTrack track = allTracks[index.row()];

    if(role == Qt::CheckStateRole && index.column() == CoUploadable)
        return ((ndev->name() != "SONY MZ-RH1 (NetMD)") || track.copyprotected()) ? Qt::Unchecked : Qt::Checked;

    if(role == Qt::DisplayRole)
    {
        switch((ncolumnum)index.column())
        {
            case CoId:
                return track.tracknum() + 1;
            case CoGroup:
                return track.group();
            case CoTitle:
                return track.title();
            case CoLength:
            {
                QTime t = track.duration();
                if(t < QTime(1,0,0))
                    return t.toString("m:ss");
                else
                    return t.toString("h:mm:ss");
            }
            case CoCodec:
                return track.codecname();
            case CoUploadable:
                return QVariant(); /* Displayed by checkbox */
        }
    }
    return QVariant();
}

int QNetMDTracksModel::rowCount(const QModelIndex &) const
{
    if(ndev == NULL)
        return 0;

    return ndev->trackCount();
}

int QNetMDTracksModel::columnCount(const QModelIndex &) const
{
    return LAST_ncolumnnum+1;
}

QString QNetMDTracksModel::open(QMDDevice * device)
{
    int i = 0;
    QString ret = "error opening net device";

    beginResetModel();
    if(ndev != NULL)
        close();

    if(device->deviceType() == NETMD_DEVICE)
    {
        ndev = static_cast<QNetMDDevice *>(device);
        ret = ndev->open();
    }

    if(!ret.isEmpty())
        close();

    /* fetch track info for all tracks first, getting track info inside data() function is very slow */
    for(; i < ndev->trackCount(); i++)
        allTracks.append(ndev->netmdTrack(i));

    endResetModel();	/* inform views that the model contents changed */
    return ret;
}

bool QNetMDTracksModel::is_open()
{
    return ndev->isOpen();
}

void QNetMDTracksModel::close()
{
    beginResetModel();

    if(ndev != NULL && ndev->isOpen())
        ndev->close();

    ndev = NULL;

    allTracks.clear();
    endResetModel();	/* inform views that the model contents changed */
}


/* himd tracks model */

QVariant QHiMDTracksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal)
        return QVariant();

    if(role == Qt::SizeHintRole)
    {
        static QFont f;
        static QFontMetrics met(f);
        switch((hcolumnum)section)
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
        switch((hcolumnum)section)
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

    QHiMDTrack track = hdev->himdTrack(index.row());

    if(role == Qt::CheckStateRole && index.column() == ColUploadable)
        return track.copyprotected() ? Qt::Unchecked : Qt::Checked;

    if(role == Qt::DisplayRole)
    {
        switch((hcolumnum)index.column())
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
    if(hdev == NULL)
        return 0;

    return hdev->trackCount();
}

int QHiMDTracksModel::columnCount(const QModelIndex &) const
{
    return LAST_hcolumnnum+1;
}

QString QHiMDTracksModel::open(QMDDevice * device)
{
    QString ret = "error opening himd device";

    beginResetModel();
    if(hdev != NULL)
        close();

    if(device->deviceType() == HIMD_DEVICE)
    {
        hdev = static_cast<QHiMDDevice *>(device);
        ret = hdev->open();
    }

    if(!ret.isEmpty())
        close();

    endResetModel();	/* inform views that the model contents changed */
    return ret;
}

bool QHiMDTracksModel::is_open()
{
    return hdev->isOpen();
}

void QHiMDTracksModel::close()
{
    beginResetModel();

    if(hdev != NULL && hdev->isOpen())
        hdev->close();

    hdev = NULL;

    endResetModel();	/* inform views that the model contents changed */
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
    beginResetModel();
    selectableExtensions = extensions;
    endResetModel();
}
