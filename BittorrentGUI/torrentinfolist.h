#ifndef TORRENTINFOLIST_H
#define TORRENTINFOLIST_H

#include <QAbstractListModel>
#include <QPointer>

#include "Client.h"

namespace Bittorrent
{

class TorrentInfoList : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit TorrentInfoList(Client* client, QPointer<QObject> parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void update();

private:
    Client* ioClient;
};

}

#endif // TORRENTINFOLIST_H
