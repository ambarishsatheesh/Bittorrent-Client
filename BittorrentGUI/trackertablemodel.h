#ifndef TRACKERTABLEMODEL_H
#define TRACKERTABLEMODEL_H

#include "storedPrevTorrents.h"
#include "Client.h"
#include "trackerObj.h"

#include <QAbstractTableModel>
#include <QVector>
#include <QTimer>
#include <QPointer>
#include <QObject>

namespace Bittorrent
{

using ptr_vec = std::unique_ptr<std::vector<trackerObj>>;


class TrackerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TrackerTableModel(ptr_vec ptr_trackerList,
                      QPointer<QObject> parent = 0);

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
    ptr_vec trackerList;

};

}

#endif // TRACKERTABLEMODEL_H
