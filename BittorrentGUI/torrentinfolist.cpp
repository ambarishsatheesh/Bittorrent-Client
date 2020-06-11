#include "torrentinfolist.h"

namespace Bittorrent
{

TorrentInfoList::TorrentInfoList(Client* client, QPointer<QObject> parent)
    : ioClient(client), QAbstractListModel(parent)
{
}

QVariant TorrentInfoList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        switch (section)
        {
        case 0:
            return "Status";
        case 1:
            return "Trackers";
        default:
            break;
        }
    }
    return QVariant();
}

int TorrentInfoList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    //return number of unique trackers
    return ioClient->workingTorrentList.infoTrackerMap.size();
}

QVariant TorrentInfoList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (index.row() >= ioClient->
            workingTorrentList.infoTrackerMap.size() ||
            index.row() < 0)
    {
        return QVariant();
    }

    // FIXME: Implement me!
    return QVariant();
}


}
