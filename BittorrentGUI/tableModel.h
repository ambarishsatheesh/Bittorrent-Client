
//#ifndef TABLEMODEL_H
//#define TABLEMODEL_H

//#endif // TABLEMODEL_H

//#include <QAbstractTableModel>

//class tableModel : public QAbstractTableModel
//{
//    Q_OBJECT
//public:
//    tableModel(QObject *parent);
//    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
//    int columnCount(const QModelIndex &parent = QModelIndex()) const;
//    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
//};


#ifndef TestModel_H
#define TestModel_H

#include "storedPrevTorrents.h"
#include "Client.h"

#include <QAbstractTableModel>
#include <QVector>
#include <QTimer>

namespace Bittorrent
{

class TestModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TestModel(Client* client, QObject *parent = 0);

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

    QVariant generateData(const std::shared_ptr<Torrent> torrent,
                          const QModelIndex &index) const;

    void addNewTorrent(std::string& fileName, std::string& buffer);
    void removeTorrent(int position, int rows);

private:
    Client* ioClientModel;

    QList<QString> tm_torrent_addedon;
    QList<QString> tm_torrent_name;
    QList<long long> tm_torrent_size;

};

}

#endif // TestModel_H
