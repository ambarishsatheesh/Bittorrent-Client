#include "torrentsortfilterproxymodel.h"

namespace Bittorrent
{

TorrentSortFilterProxyModel::TorrentSortFilterProxyModel(
        QPointer<QObject> parent)
{

}

bool TorrentSortFilterProxyModel::filterAcceptsRow(
        int sourceRow, const QModelIndex& sourceParent) const
{
    //get index of given row at the torrent Name column (2)
    QModelIndex torrentNameIndex = sourceModel()->index(
                sourceRow, 2, sourceParent);

    //regExp set in MainWindow
    return sourceModel()->data(torrentNameIndex).toString().
            contains(filterRegExp());
}


}



