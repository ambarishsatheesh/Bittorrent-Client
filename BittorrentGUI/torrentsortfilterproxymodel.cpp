#include "torrentsortfilterproxymodel.h"

namespace Bittorrent
{

TorrentSortFilterProxyModel::TorrentSortFilterProxyModel(Client* client,
        QPointer<QObject> parent)
    : ioClientModel(client)
{

}

bool TorrentSortFilterProxyModel::filterAcceptsRow(
        int sourceRow, const QModelIndex& sourceParent) const
{
    //infoHashList is filled when MainWindow::trackerListItemSelected slot is
    //called so tracker filter is run. Otherwise search filter is run.
    if (!infoHashList.isEmpty())
    {
        for (auto infoHash : infoHashList)
        {
            if (infoHash.toStdString() == ioClientModel->
                    WorkingTorrentList.torrentList.at(sourceRow)->
                    hashesData.hexStringInfoHash)
            {
                return true;
            }
        }

        return false;
    }
    //search filter
    else
    {
        //get index of given row at the torrent Name column (2)
        QModelIndex torrentNameIndex = sourceModel()->index(
                    sourceRow, 2, sourceParent);

        //regExp set in MainWindow
        return sourceModel()->data(torrentNameIndex).toString().
                contains(filterRegExp());
    }
}


}



