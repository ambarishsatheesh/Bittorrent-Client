#include "torrentinfolist.h"

namespace Bittorrent
{

TorrentInfoList::TorrentInfoList(Client* client,
                                 QPointer<QObject> parent)
    : QAbstractListModel(parent), ioClient(client)
{
}

QVariant TorrentInfoList::headerData(int section,
                                     Qt::Orientation orientation, int role) const
{
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

QVariant TorrentInfoList::data(const QModelIndex &index,
                               int role) const
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

    //use list of keys from map
    auto trackerAdd = ioClient->
            workingTorrentList.infoTrackerMap.keys().at(index.row());

    return trackerAdd + " (" + QString::number(ioClient->
                workingTorrentList.infoTrackerMap[trackerAdd]) + ")";
}

void TorrentInfoList::update()
{
    emit dataChanged(QModelIndex(), QModelIndex());
}


}
