
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


#ifndef MYMODEL_H
#define MYMODEL_H

#include "storedPrevTorrents.h"
#include <QAbstractTableModel>
#include <QTimer>

class TestModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TestModel(QObject *parent = 0);

    void populateData(const storedPrevTorrents storedTor);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    QList<QString> tm_torrent_addedon;
    QList<QString> tm_torrent_name;
    QList<long long> tm_torrent_size;

};

#endif // MYMODEL_H
