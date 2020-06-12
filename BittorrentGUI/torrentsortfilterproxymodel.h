#ifndef TORRENTSORTFILTERPROXYMODEL_H
#define TORRENTSORTFILTERPROXYMODEL_H

#include "tableModel.h"

#include <QSortFilterProxyModel>
#include <QPointer>
#include <QObject>

namespace Bittorrent
{

class TorrentSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TorrentSortFilterProxyModel(Client* client, QPointer<QObject> parent = 0);

    QList<QString> infoHashList;


protected:
    bool filterAcceptsRow(
            int sourceRow, const QModelIndex &sourceParent) const override;

private:
    Client* ioClientModel;
};

}

#endif // TORRENTSORTFILTERPROXYMODEL_H
