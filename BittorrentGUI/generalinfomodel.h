#ifndef GENERALINFOMODEL_H
#define GENERALINFOMODEL_H

#include <QAbstractItemModel>
#include <QPointer>

#include "Client.h"

namespace Bittorrent
{

class generalInfoModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit generalInfoModel(Client* client, QPointer<QObject> parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant generateData(const QModelIndex &index) const;

private:
    Client* ioClient;
    int columnSize;
};

#endif // GENERALINFOMODEL_H


}

