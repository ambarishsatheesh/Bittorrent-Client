#ifndef TORRENTSORTFILTERPROXYMODEL_H
#define TORRENTSORTFILTERPROXYMODEL_H

#include "tableModel.h"

#include <QSortFilterProxyModel>

namespace Bittorrent
{

class TorrentSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TorrentSortFilterProxyModel(QObject* parent = 0);


protected:
    bool filterTorrentName(
            int sourceRow, const QModelIndex &sourceParent) const;

private:
    Client* ioClientModel;
};

}

#endif // TORRENTSORTFILTERPROXYMODEL_H
