#include "generalinfomodel.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <locale>
#include <sstream>
#include <QPointer>


namespace Bittorrent
{

generalInfoModel::generalInfoModel(Client* client, QPointer<QObject> parent)
    : QAbstractItemModel(parent), ioClient(client), columnSize{17}
{

}

//int generalInfoModel::rowCount(const QModelIndex &parent) const
//{
//    return parent.isValid() ?
//                0 : ioClient->workingTorrentList.torrentList.size();
//}

//int generalInfoModel::columnCount(const QModelIndex &parent) const
//{
//    return parent.isValid() ? 0 : columnSize;
//}

//QVariant generalInfoModel::data(const QModelIndex &index, int role) const
//{
//    if (!index.isValid() || role != Qt::DisplayRole)
//    {
//        return QVariant();
//    }

//    if (index.row() >= ioClient->workingTorrentList.torrentList.size() ||
//            index.row() < 0)
//    {
//        return QVariant();
//    }

//    return generateData(index);
//}


//QVariant generalInfoModel::headerData(int section,
//                               Qt::Orientation orientation, int role) const
//{
//    return QVariant();
//}

//bool generalInfoModel::setData(const QModelIndex &index,
//                        const QVariant &value, int role)
//{
//        if (role == Qt::DisplayRole)
//        {
//            emit dataChanged(index, index);
//        }

//        return true;
// }



QVariant generalInfoModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const
{
    return QVariant();
}

QModelIndex generalInfoModel::index(int row, int column,
                                    const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent))
    {
        return createIndex(row, column);
    }
    return QModelIndex();
}

QModelIndex generalInfoModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int generalInfoModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ?
                0 : ioClient->
                workingTorrentList.torrentList.size();
}

int generalInfoModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : columnSize;
}

bool generalInfoModel::setData(const QModelIndex &index,
                        const QVariant &value, int role)
    {
        if (role == Qt::DisplayRole)
        {
            emit dataChanged(index, index);
        }

        return true;
 }

QVariant generalInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    if (index.row() >= ioClient->workingTorrentList.torrentList.size() ||
            index.row() < 0)
    {
        return QVariant();
    }

    return generateData(index);
}

QVariant generalInfoModel::generateData(const QModelIndex &index) const
{
    auto entry = ioClient->workingTorrentList.torrentList.at(index.row());

    //order of columns is arbitrary since this is not for a table
    switch (index.column())
    {
    //INFORMATION SECTION

    //Total Size
    case 0:
    {
        return QString::fromStdString(humanReadableBytes(
                        entry->piecesData.totalSize));
    }
    //Added On
    case 1:
        return ioClient->workingTorrentList.addedOnList.at(index.row());
    //Created On
    case 2:
        //convert ptime to required string format
    {
        auto time = entry->generalData.creationDate;
        //no need to delete facet - locale is responsible for this
        auto* facet = new boost::posix_time::time_facet();
        facet->format("%Y/%m/%d %H:%M");
        std::stringstream stream;
        stream.imbue(std::locale(std::locale::classic(), facet));
        stream << time;
        return QString::fromStdString(stream.str());
    }
    //Created By
    case 3:
        return QString::fromStdString(entry->generalData.createdBy);
    //Comment
    case 4:
        return QString::fromStdString(entry->generalData.comment);
    //Completed On
    case 5:
        //implement properly
        return 0;
    //Pieces
    case 6:
    {
        auto pieces = QString::number(entry->piecesData.pieceCount);
        auto pSize = QString::fromStdString(entry->
                                       piecesData.readablePieceSize());
        auto pHave = entry->statusData.verifiedPiecesCount();

        return pieces + " x " + pSize + " (have " + pHave + ")";
    }
    //Torrent Hash
    case 7:
        return QString::fromStdString(entry->hashesData.hexStringInfoHash);
    //Save Path
    case 8:
        return QString::fromStdString(entry->generalData.downloadDirectory);

    //TRANSFERS SECTION

    //Time Active
    case 9:
        //implement properly
        return 0;
    //Downloaded
    case 10:
        return QString::fromStdString(humanReadableBytes(
                                          entry->statusData.downloaded()));
    //download speed
    case 11:
        //implement properly
        return 0;
    //ETA
    case 12:
        //implement properly
        return 0;
    //Uploaded
    case 13:
        return QString::fromStdString(humanReadableBytes(
                                          entry->statusData.uploaded()));
    //upload speed
    case 14:
        //implement properly
        return 0;
    //Connections
    case 15:
        //implement properly
        return 0;
    //seeds
    case 16:
        //implement properly
        return 0;
    //peers
    case 17:
        //implement properly
        return 0;
    default:
        break;
    }
}

}
