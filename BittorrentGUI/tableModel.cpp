#include "tableModel.h"
#include "loguru.h"
#include "Utility.h"

#include <Qtime>
#include <QString>

namespace Bittorrent
{

TorrentTableModel::TorrentTableModel(Client* client, QPointer<QObject> parent)
    : QAbstractTableModel(parent), ioClientModel(client)
{
}

int TorrentTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ?
                0 : ioClientModel->
                WorkingTorrents.torrentList.size();
}

int TorrentTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 16;
}

QVariant TorrentTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (index.row() >=
            ioClientModel->WorkingTorrents.torrentList.size() ||
            index.row() < 0)
    {
        return QVariant();
    }

    return generateData(index);
}


QVariant TorrentTableModel::headerData(int section,
                               Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section)
        {
        case 0:
            return QString("Added On");
        case 1:
            return QString("Priority");
        case 2:
            return QString("Name");
        case 3:
            return QString("Size");
        case 4:
            return QString("Progress");
        case 5:
            return QString("Status");
        case 6:
            return QString("Seeds");
        case 7:
            return QString("Peers");
        case 8:
            return QString("Download Speed");
        case 9:
            return QString("Upload Speed");
        case 10:
            return QString("ETA");
        case 11:
            return QString("Ratio");
        case 12:
            return QString("Tracker");
        case 13:
            return QString("Downloaded");
        case 14:
            return QString("Uploaded");
        case 15:
            return QString("Time Active");
        default:
            break;
        }
    }
    return QVariant();
}

bool TorrentTableModel::setData(const QModelIndex &index,
                        const QVariant &value, int role)
    {
        if (role == Qt::DisplayRole)
        {
            emit dataChanged(index, index);
        }

        return true;
 }

QVariant TorrentTableModel::generateData(const QModelIndex &index) const
{
    using namespace utility;

    auto entry = ioClientModel->WorkingTorrents.torrentList.at(index.row());

    switch (index.column())
    {
    //Added on
    case 0:
            return ioClientModel->
                    WorkingTorrents.addedOnList.at(index.row());
    //Priority
    case 1:
        return ioClientModel->
                WorkingTorrents.torrentList.at(index.row())->clientRank;
    //Name
    case 2:
        return QString::fromStdString(entry->generalData.fileName);
    //Size
    case 3:
        return QString::fromStdString(
                    humanReadableBytes(entry->piecesData.totalSize));
    //Progress
    case 4:
    {
        auto downloadedBytes = entry->statusData.downloaded();

        auto totalBytes = entry->piecesData.totalSize;

        return 100 * (downloadedBytes/totalBytes);
    }
    //Status
    case 5:
    {
        if (ioClientModel->
                WorkingTorrents.torrentList.at(index.row())->
                statusData.currentState ==
                TorrentStatus::currentStatus::started)
        {
            return "Downloading";
        }
        else if (ioClientModel->
                 WorkingTorrents.torrentList.at(index.row())->
                 statusData.currentState ==
                 TorrentStatus::currentStatus::stopped)
        {
            if (ioClientModel->
                    WorkingTorrents.torrentList.at(index.row())->
                    statusData.downloaded() == 0)
            {
                return "Stalled";
            }

            return "Paused";
        }
        else if (ioClientModel->
                 WorkingTorrents.torrentList.at(index.row())->
                 statusData.currentState ==
                 TorrentStatus::currentStatus::completed)
        {
            if (ioClientModel->
                    WorkingTorrents.torrentList.at(index.row())->isSeeding)
            {
                return "Seeding";
            }

            return "Completed";
        }
        else
        {
            return "Queued";
        }
    }
    //seeds
    case 6:
        //implement properly
        return 0;
    //peers
    case 7:
        //implement properly
        return 0;
    //download speed
    case 8:
        //implement properly
        return 0;
    //upload speed
    case 9:
        //implement properly
        return 0;
    //ETA
    case 10:
        //implement properly
        return 0;
    //Ratio
    case 11:
    {
        auto downloadedBytes = entry->statusData.downloaded();
        if (downloadedBytes != 0)
        {
            return entry->statusData.uploaded / downloadedBytes;
        }
        return 0;
    }
    //Tracker
    case 12:
        //implement properly
        return 0;
    //Downloaded
    case 13:
        return QString::fromStdString(humanReadableBytes(entry->
                statusData.downloaded()));
    //Uploaded
    case 14:
        return QString::fromStdString(humanReadableBytes(entry->
                statusData.uploaded));
    //Time Active
    case 15:
        //implement properly
        return 0;
    default:
        break;
    }
}

void TorrentTableModel::addNewTorrent(Torrent* modifiedTorrent)
{
    //get last row
    const int newRow =
            ioClientModel->WorkingTorrents.torrentList.size();

    //duplicate torrent validation (returns torrent name if duplicate)
    auto duplicateName = QString::fromStdString(ioClientModel->
                           WorkingTorrents.isDuplicateTorrent(modifiedTorrent));

    if (duplicateName.isEmpty())
    {
        //begin row insert operation before changing data
        beginInsertRows(QModelIndex(), newRow, newRow);

        ioClientModel->
                    WorkingTorrents.addNewTorrent(modifiedTorrent);

        endInsertRows();
    }
    else
    {
        //send signal notifying of torrent duplication
        emit duplicateTorrentSig(duplicateName);

        LOG_F(INFO, "Duplicate torrent '%s'!",
              duplicateName.toStdString().c_str());
    }
}


void TorrentTableModel::removeTorrent(int position)
{
    auto deletedTorName =
            ioClientModel->WorkingTorrents.torrentList.at(position)->
            generalData.fileName;

    beginRemoveRows(QModelIndex(), position, position);

    ioClientModel->
            WorkingTorrents.removeTorrent(position);

    endRemoveRows();

    LOG_F(INFO, "Removed torrent '%s' from client!", deletedTorName.c_str());

}


}
