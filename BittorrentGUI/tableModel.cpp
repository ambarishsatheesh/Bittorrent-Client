#include "tableModel.h"
#include "loguru.h"
#include "Utility.h"

#include <Qtime>

namespace Bittorrent
{

TestModel::TestModel(Client* client, QPointer<QObject> parent)
    : QAbstractTableModel(parent), ioClientModel(client)
{
}

int TestModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ?
                0 : ioClientModel->
                workingTorrentList.torrentList.size();
}

int TestModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 16;
}

QVariant TestModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (index.row() >=
            ioClientModel->workingTorrentList.torrentList.size() ||
            index.row() < 0)
    {
        return QVariant();
    }

    return generateData(index);
}


QVariant TestModel::headerData(int section,
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

bool TestModel::setData(const QModelIndex &index,
                        const QVariant &value, int role)
    {
        if (role == Qt::DisplayRole)
        {
            emit dataChanged(index, index);
        }

        return true;
 }

QVariant TestModel::generateData(const QModelIndex &index) const
{
    using namespace utility;

    switch (index.column())
    {
    //Added on
    case 0:
            return ioClientModel->
                    workingTorrentList.addedOnList.at(index.row());
    //Priority
    case 1:
        //implement priority properly
        return 1;
    //Name
    case 2:
        return QString::fromStdString(ioClientModel->
                                      workingTorrentList.torrentList.
                                      at(index.row())->generalData.fileName);
    //Size
    case 3:
        return QString::fromStdString(
                    humanReadableBytes(ioClientModel->
                                       workingTorrentList.torrentList.
                                       at(index.row())->piecesData.totalSize));
    //Progress
    case 4:
    {
        auto downloadedBytes = ioClientModel->
                workingTorrentList.torrentList.at(index.row())->
                statusData.downloaded();

        auto totalBytes = ioClientModel->
                workingTorrentList.torrentList.at(index.row())->
                piecesData.totalSize;

        return downloadedBytes/totalBytes;
    }
    //Status
    case 5:
         //implement using progress
        return 0;
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
        return ioClientModel->
                workingTorrentList.torrentList.at(index.row())->
                statusData.uploaded() / ioClientModel->
                workingTorrentList.torrentList.at(index.row())->
                statusData.downloaded();
    //Tracker
    case 12:
        //implement properly
        return 0;
    //Downloaded
    case 13:
        return QString::fromStdString(humanReadableBytes(ioClientModel->
                workingTorrentList.torrentList.at(index.row())->
                statusData.downloaded()));
    //Uploaded
    case 14:
        return QString::fromStdString(humanReadableBytes(ioClientModel->
                workingTorrentList.torrentList.at(index.row())->
                statusData.uploaded()));
    //Time Active
    case 15:
        //implement properly
        return 0;
    default:
        break;
    }
}

void TestModel::addNewTorrent(std::string& fileName, std::string& buffer)
{
    //get last row
    const int newRow =
            ioClientModel->workingTorrentList.torrentList.size();

    //begin row insert operation before changing data
    beginInsertRows(QModelIndex(), newRow, newRow);

    //load torrent
    ioClientModel->workingTorrentList.addNewTorrent(fileName, buffer);

    //no need to explicitly call emit dataChanged() - this will update view by itself
    endInsertRows();

    LOG_F(INFO, "Added torrent \"%s\"",
          ioClientModel->workingTorrentList.torrentList.at(newRow)->
          generalData.fileName.c_str());
}

void TestModel::removeTorrent(int position)
{
    auto deletedTorName =
            ioClientModel->workingTorrentList.torrentList.at(position)->
            generalData.fileName;

    beginRemoveRows(QModelIndex(), position, position);

    ioClientModel->
            workingTorrentList.removeTorrent(position);

    endRemoveRows();

    LOG_F(INFO, "Removed torrent \"%s\"", deletedTorName.c_str());

}


}
