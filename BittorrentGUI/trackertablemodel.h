#ifndef TRACKERTABLEMODEL_H
#define TRACKERTABLEMODEL_H

#include "storedPrevTorrents.h"
#include "Client.h"

#include <QAbstractTableModel>
#include <QVector>
#include <QTimer>
#include <QPointer>
#include <QObject>

namespace Bittorrent
{

class TrackerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TrackerTableModel(Client* client, QPointer<QObject> parent = 0);

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

signals:


private:
    Client* ioClientModel;

    QList<QString> tm_torrent_addedon;
    QList<QString> tm_torrent_name;
    QList<long long> tm_torrent_size;

};

}

#endif // TRACKERTABLEMODEL_H
