#include "generalinfomodel.h"

generalInfoModel::generalInfoModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QVariant generalInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
}

QModelIndex generalInfoModel::index(int row, int column, const QModelIndex &parent) const
{
    // FIXME: Implement me!
}

QModelIndex generalInfoModel::parent(const QModelIndex &index) const
{
    // FIXME: Implement me!
}

int generalInfoModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
}

int generalInfoModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
}

QVariant generalInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}
