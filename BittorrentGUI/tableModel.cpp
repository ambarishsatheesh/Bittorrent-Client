
//#include "tableModel.h"

//tableModel::tableModel(QObject *parent)
//    :QAbstractTableModel(parent)
//{
//}

//int tableModel::rowCount(const QModelIndex & /*parent*/) const
//{
//   return 2;
//}

//int tableModel::columnCount(const QModelIndex & /*parent*/) const
//{
//    return 3;
//}

//QVariant tableModel::data(const QModelIndex &index, int role) const
//{
//    if (role == Qt::DisplayRole)
//    {
//       return QString("Row%1, Column%2")
//                   .arg(index.row() + 1)
//                   .arg(index.column() +1);
//    }
//    return QVariant();
//}


#include "tableModel.h"
#include "loguru.h"


#include <Qtime>

TestModel::TestModel(QObject *parent) : QAbstractTableModel(parent)
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
    return tm_torrent_name.length();
}

int TestModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 16;
}

QVariant TestModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }
    if (index.column() == 0) {
        return tm_torrent_name[index.row()];
    } else if (index.column() == 1) {
        return tm_torrent_size[index.row()];
    }
    return QVariant();
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
