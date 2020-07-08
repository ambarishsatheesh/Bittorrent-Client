#include "tableModel.h"
#include "loguru.h"
#include "Utility.h"

#include <Qtime>
#include <QString>

namespace Bittorrent
{

TorrentTableModel::TorrentTableModel(Client* client, QPointer<QObject> parent)
    : QAbstractTableModel(parent), ioClientModel(client), timer{new QTimer(this)}
{
    //timer to refresh table every second
    connect(timer, &QTimer::timeout , this, &TorrentTableModel::timerHit);
    timer->start(2000);
}

void TorrentTableModel::timerHit()
{
    emit this->dataChanged(QModelIndex(), QModelIndex());
}

int TorrentTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ?
                0 : ioClientModel->
                WorkingTorrents.torrentList.size();
}

int TorrentTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 14;
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
            return QString("Downloaded");
        case 13:
            return QString("Uploaded");
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

    std::shared_ptr<Torrent> torrent =
            ioClientModel->WorkingTorrents.torrentList.at(index.row());

    switch (index.column())
    {
    //Added on
    case 0:
            return ioClientModel->
                    WorkingTorrents.addedOnList.at(index.row());
    //Priority
    case 1:
        return torrent->clientRank;
    //Name
    case 2:
        return QString::fromStdString(torrent->generalData.fileName);
    //Size
    case 3:
        return QString::fromStdString(humanReadableBytes(torrent->
                                                         piecesData.totalSize));
    //Progress
    case 4:
    {
        return std::floor(100 * (torrent->statusData.downloaded() /
                      static_cast<float>(torrent->piecesData.totalSize)));
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
        else if (torrent->statusData.currentState ==
                 TorrentStatus::currentStatus::completed)
        {
            if (torrent->statusData.isSeeding)
            {
                return "Seeding";
            }

            return "Completed";
        }
        else if (torrent->statusData.isVerifying)
        {
            return "Verifying";
        }
        else if (torrent->statusData.currentState ==
                 TorrentStatus::currentStatus::stopped)
        {
            if (torrent->statusData.downloadSpeed == 0)
            {
                return "Stalled";
            }

            return "Paused";
        }
    }
    //seeds
    case 6:
    {
        return ioClientModel->WorkingTorrents.seedersMap.count(
                    torrent->hashesData.urlEncodedInfoHash);
    }
    //peers
    case 7:
        return ioClientModel->WorkingTorrents.leechersMap.count(
                    torrent->hashesData.urlEncodedInfoHash);
    //download speed
    case 8:
        return QString::fromStdString(humanReadableBytes(
                                          torrent->
                                          statusData.downloadSpeed)) + "/s";
    //upload speed
    case 9:
        //implement properly
        return 0;
    //ETA
    case 10:
    {
        if (torrent->statusData.currentState ==
                TorrentStatus::currentStatus::completed ||
                torrent->statusData.currentState ==
                                TorrentStatus::currentStatus::stopped ||
                torrent->statusData.downloaded() == 0 ||
                torrent->statusData.downloadSpeed == 0)
        {
            return QString("\u221E");
        }
        else
        {
            return QDateTime::currentDateTime().addSecs(
                        torrent->statusData.remaining() /
                        torrent->statusData.downloadSpeed).toString(
                        "hh:mm:ss dd/MM/yyyy");
        }
    }
    //Ratio
    case 11:
    {
        if (torrent->statusData.downloaded() != 0)
        {
            return ioClientModel->
                    WorkingTorrents.torrentList.at(index.row())->
                    statusData.uploaded / torrent->statusData.downloaded();
        }
        return 0;
    }
    //Downloaded
    case 12:
        return QString::fromStdString(humanReadableBytes(
                                          torrent->statusData.downloaded()));
    //Uploaded
    case 13:
        return QString::fromStdString(humanReadableBytes(
                                          torrent->statusData.uploaded));
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
