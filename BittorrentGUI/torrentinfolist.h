#ifndef TORRENTINFOLIST_H
#define TORRENTINFOLIST_H

#include <QAbstractListModel>

class torrentInfoList : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit torrentInfoList(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
};

#endif // TORRENTINFOLIST_H
