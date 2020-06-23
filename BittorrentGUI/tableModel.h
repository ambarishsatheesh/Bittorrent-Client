#ifndef TestModel_H
#define TestModel_H

#include "storedPrevTorrents.h"
#include "Client.h"
#include "TorrentManipulation.h"

#include <QAbstractTableModel>
#include <QVector>
#include <QTimer>
#include <QPointer>
#include <QObject>

namespace Bittorrent
{

class TorrentTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TorrentTableModel(Client* client, QPointer<QObject> parent = 0);

    void populateData(const storedPrevTorrents storedTor);

    int rowCount(const QModelIndex &parent =
            QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent =
            QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    QVariant generateData(const QModelIndex &index) const;

    void addNewTorrent(Torrent* modifiedTorrent);
    void removeTorrent(int position);

signals:
    void duplicateTorrentSig(QString torrentName);

private:
    Client* ioClientModel;

};

}

#endif // TestModel_H
