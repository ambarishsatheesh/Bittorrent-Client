#include "tableModel.h"
#include "loguru.h"

#include <Qtime>

namespace Bittorrent
{

TestModel::TestModel(Client* client, QObject *parent)
    : QAbstractTableModel(parent), ioClientModel(client)
{
}

// Create a method to populate the model with data:
void TestModel::populateData(const storedPrevTorrents storedTor)
{

    tm_torrent_addedon = storedTor.torrentAddedOn;
    tm_torrent_name = storedTor.torrentName;
    //tm_torrent_size.clear();
    tm_torrent_size = storedTor.torrentSize;
    return;
}

int TestModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return ioClientModel->workingTorrentList.torrentList.size();
}

int TestModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 16;
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

    const auto &singleTorrent =
            ioClientModel->workingTorrentList.torrentList.at(index.row());

    return generateData(singleTorrent, index);
}


QVariant TestModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0)
        {
            return QString("Added On");
        }
        else if (section == 1)
        {
            return QString("Priority");
        }
        else if (section == 2)
        {
            return QString("Name");
        }
        else if (section == 3)
        {
            return QString("Size");
        }
        else if (section == 4)
        {
            return QString("Progress");
        }
        else if (section == 5)
        {
            return QString("Status");
        }
        else if (section == 6)
        {
            return QString("Seeds");
        }
        else if (section == 7)
        {
            return QString("Peers");
        }
        else if (section == 8)
        {
            return QString("Download Speed");
        }
        else if (section == 9)
        {
            return QString("Upload Speed");
        }
        else if (section == 10)
        {
            return QString("ETA");
        }
        else if (section == 11)
        {
            return QString("Ratio");
        }
        else if (section == 12)
        {
            return QString("Tracker");
        }
        else if (section == 13)
        {
            return QString("Downloaded");
        }
        else if (section == 14)
        {
            return QString("Uploaded");
        }
        else if (section == 15)
        {
            return QString("Time Active");
        }
    }
    return QVariant();
}

bool TestModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::DisplayRole)
        {
            LOG_F(INFO, "Index.row: %d", index.row());
            tm_torrent_name[index.row()] = value.toString();  //  set the new data

            emit dataChanged(index, index);     //  explicitly emit dataChanged signal, notifies TreeView to update by
                                                //  calling this->data(index, Qt::DisplayRole)
        }

        return true;
 }

QVariant TestModel::generateData(const std::shared_ptr<Torrent> torrent,
                                 const QModelIndex &index) const
{
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
        return QString::fromStdString(torrent->generalData.fileName);
    //Size
    case 3:
        return torrent->piecesData.totalSize;
    //Progress
    case 4:
        //implement using delegates
        return 0;
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
        //implement properly
        return 0;
    //Tracker
    case 12:
        //implement properly
        return 0;
    //Downloaded
    case 13:
        //implement properly
        return 0;
    //Uploaded
    case 14:
        //implement properly
        return 0;
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
    const int newRow = this->rowCount();

    //begin row insert operation before changing data
    beginInsertRows(QModelIndex(), newRow, newRow);

    //load torrent
    ioClientModel->workingTorrentList.addNewTorrent(fileName, buffer);

    //no need to explicitly call emit dataChanged() - this will update view by itself
    endInsertRows();
}

bool TestModel::removeRows(int position, int rows, const QModelIndex &index)
{
//    Q_UNUSED(index);
//    beginRemoveRows(QModelIndex(), position, position + rows - 1);

//    for (int row = 0; row < rows; ++row)
//        contacts.removeAt(position);

//    endRemoveRows();
//    return true;
}


}
