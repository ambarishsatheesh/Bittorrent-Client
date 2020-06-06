#include "tableModel.h"

tableModel::tableModel(QObject *parent)
    :QAbstractTableModel(parent)
{
}

int tableModel::rowCount(const QModelIndex & /*parent*/) const
{
   return 2;
}

int tableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

QVariant tableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
       return QString("Row%1, Column%2")
                   .arg(index.row() + 1)
                   .arg(index.column() +1);
    }
    return QVariant();
}
