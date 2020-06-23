#include "contenttreemodel.h"

#include <QHashFunctions>

ContentTreeModel::ContentTreeModel(const QVector<std::string>& data,
                                   QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new ContentTree({tr("Name"), tr("Size")}, 0);
    setupModelData(data, rootItem);
}

ContentTreeModel::~ContentTreeModel()
{
    delete rootItem;
}

QVariant ContentTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex ContentTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ContentTree *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ContentTree*>(parent.internalPointer());

    ContentTree *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex ContentTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ContentTree *childItem = static_cast<ContentTree*>(index.internalPointer());
    ContentTree *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ContentTreeModel::rowCount(const QModelIndex &parent) const
{
    ContentTree *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ContentTree*>(parent.internalPointer());

    return parentItem->childCount();
}

int ContentTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<ContentTree*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}

QVariant ContentTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    ContentTree *item = static_cast<ContentTree*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags ContentTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

int ContentTreeModel::findNode(unsigned int& hash,
                               const QList<ContentTree*>& tList)
{
    for (int idx = 0; idx < tList.size(); ++idx)
    {
        unsigned int z = tList.at(idx)->getIndex();
        if (z == hash)
        {
            return idx;
        }
    }

    return -1;
}

void ContentTreeModel::setupModelData(const QVector<std::string>& data,
                                      ContentTree *parent)
{
    QList<ContentTree*> parents;
    parents << parent;

    for (int i = 0; i < data.size(); ++i)
    {
        QString name = QString::fromStdString(data.at(i));

       QStringList nodeString = name.split("/", QString::SkipEmptyParts);

       QString temppath = "";

       int lastidx = 0;
       for(int node = 0; node < nodeString.count(); ++node)
       {
           temppath += nodeString.at(node);
           if(node != nodeString.count() - 1)
               temppath += "/";

           unsigned int hash = qHash(temppath);
           QList<QVariant> columnData;

           columnData << nodeString.at(node);

           int idx = findNode(hash, parents);

           if(idx != -1)
           {
                lastidx = idx;
           }
           else
           {
               if(lastidx != -1)
               {
                   parents.at(lastidx)->appendChild(new ContentTree(columnData, hash, parents.at(lastidx)));
                   parents <<  parents.at(lastidx)->child( parents.at(lastidx)->childCount()-1);
                   lastidx = -1;
               }
               else
               {
                   parents.last()->appendChild(new ContentTree(columnData, hash, parents.last()));
                   parents <<  parents.last()->child( parents.last()->childCount()-1);
               }
           }
       }
    }
}
