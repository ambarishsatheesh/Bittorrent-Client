#include "trackertablemodel.h"
#include "loguru.h"
#include "Utility.h"

#include <Qtime>
#include <QString>

namespace Bittorrent
{

TrackerTableModel::TrackerTableModel(Client* client, QPointer<QObject> parent)
    : QAbstractTableModel(parent), ioClientModel(client)
{
}

int TrackerTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ?
                0 : ioClientModel->
                workingTorrentList.torrentList.size();
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

    if (index.row() >=
            ioClientModel->workingTorrentList.torrentList.size() ||
            index.row() < 0)
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
            return QString("Peers");
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
        return 0;
    //Status
    case 1:
        return 0;
    //Received Peers
    case 2:
        return 0;
    //Seeds
    case 3:
        return 0;
    //Peers
    case 4:
        return 0;
    //Message
    case 5:
        return 0;
    default:
        break;
    }
}


}
