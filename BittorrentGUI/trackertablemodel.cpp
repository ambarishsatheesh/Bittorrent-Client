#include "trackertablemodel.h"
#include "loguru.h"
#include "Utility.h"

#include <QString>

namespace Bittorrent
{

TrackerTableModel::TrackerTableModel(tracker_vec ptr_trackerList,
        QPointer<QObject> parent)
    : QAbstractTableModel(parent),
      trackerList(std::move(ptr_trackerList))
{
}

int TrackerTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ?
                0 : trackerList->size();
}

int TrackerTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 6;
}

QVariant TrackerTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (index.row() >= trackerList->size() || index.row() < 0)
    {
        return QVariant();
    }

    return generateData(index);
}


QVariant TrackerTableModel::headerData(int section,
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
            return QString("URL");
        case 1:
            return QString("Status");
        case 2:
            return QString("Received Peers");
        case 3:
            return QString("Seeds");
        case 4:
            return QString("Leechers");
        case 5:
            return QString("Message");
        default:
            break;
        }
    }
    return QVariant();
}

bool TrackerTableModel::setData(const QModelIndex &index,
                        const QVariant &value, int role)
{
        if (role == Qt::DisplayRole)
        {
            emit dataChanged(index, index);
        }

        return true;
 }

QVariant TrackerTableModel::generateData(const QModelIndex &index) const
{
    using namespace utility;

    switch (index.column())
    {
    //URL
    case 0:
        return QString::fromStdString(
                    trackerList->at(index.row()).trackerAddress);
    //Status
    case 1:
        return 0;
    //Received Peers
    case 2:
    {
        //if no connection to tracker has been made
        if (trackerList->at(index.row()).seeders &&
                trackerList->at(index.row()).leechers ==
                std::numeric_limits<int>::min())
        {
            return 0;
        }
        else
        {
            return trackerList->at(index.row()).seeders +
                    trackerList->at(index.row()).leechers;
        }
    }
    //Seeds
    case 3:
        //if no connection to tracker has been made
        if (trackerList->at(index.row()).seeders ==
                std::numeric_limits<int>::min())
        {
            return 0;
        }
        else
        {
            return trackerList->at(index.row()).seeders;
        }
    //Leechers
    case 4:
        //if no connection to tracker has been made
        if (trackerList->at(index.row()).seeders ==
                std::numeric_limits<int>::min())
        {
            return 0;
        }
        else
        {
            return trackerList->at(index.row()).leechers;
        }
    //Message
    case 5:
        return 0;
    default:
        break;
    }
}


}
