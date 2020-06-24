#include "contenttreemodel.h"

#include <QHashFunctions>

namespace Bittorrent {

ContentTreeModel::ContentTreeModel(Client* client,
                                   std::vector<fileObj>* ptr_fileList,
                                   QObject *parent)
    : QAbstractItemModel(parent), fileList{ptr_fileList}, ioClient{client}
{
    rootItem = new ContentTree({tr("Name"), tr("Size")}, 0);
    setupModelData(rootItem);
}

//only for add torrent dialog
ContentTreeModel::ContentTreeModel(const Torrent& modifiedTorrent,
                                   QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new ContentTree({tr("Name"), tr("Size")}, 0);
    setupModelData(modifiedTorrent, rootItem);
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

void ContentTreeModel::setupModelData(ContentTree *parent)
{
    QList<ContentTree*> parents;
    parents << parent;

    //copy to new vector to allow modification of file names
    auto fileListCopy = *fileList;

    //add top level parent folder to each file name
    for (auto& file : fileListCopy)
    {
        file.filePath.insert(0, ioClient->WorkingTorrents.torrentList.back()->
                             generalData.fileName + "/");
    }

    for (int i = 0; i < fileListCopy.size(); ++i)
    {
        QString name = QString::fromStdString(fileListCopy.at(i).filePath);

       QStringList nodeString = name.split("/");

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
                   //if not a parent, get file size
                   if (temppath.lastIndexOf('/') != temppath.size() - 1)
                   {
                       columnData << QString::fromStdString(
                                        humanReadableBytes(
                                            fileListCopy.at(i).fileSize));
                   }

                   parents.at(lastidx)->appendChild(
                               new ContentTree(columnData,
                                               hash, parents.at(lastidx)));
                   parents <<  parents.at(lastidx)->child(
                                   parents.at(lastidx)->childCount()-1);
                   lastidx = -1;
               }
               else
               {
                   //if not a parent, get file size
                   if (temppath.lastIndexOf('/') != temppath.size() - 1)
                   {
                       columnData << QString::fromStdString(
                                        humanReadableBytes(
                                            fileListCopy.at(i).fileSize));
                   }

                   parents.last()->appendChild(
                               new ContentTree(columnData,
                                               hash, parents.last()));
                   parents <<  parents.last()->child(
                                   parents.last()->childCount()-1);
               }
           }
       }
    }
}

void ContentTreeModel::setupModelData(const Torrent& modifiedTorrent,
                                      ContentTree *parent)
{
    QList<ContentTree*> parents;
    parents << parent;

    //copy to new vector to allow modification of file names
    auto fileListCopy = modifiedTorrent.fileList;

    //add top level parent folder to each file name
    for (auto& file : fileListCopy)
    {
        file.filePath.insert(0, modifiedTorrent.generalData.fileName + "/");
    }

    for (int i = 0; i < fileListCopy.size(); ++i)
    {
        QString name = QString::fromStdString(
                    fileListCopy.at(i).filePath);

        QStringList nodeString = name.split("/");

        QString temppath = "";

        int lastidx = 0;
        for(int node = 0; node < nodeString.count(); ++node)
        {
            temppath += nodeString.at(node);
            if(node != nodeString.count() - 1)
            {
                temppath += "/";
            }

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
                   //if not a parent, get file size
                   if (temppath.lastIndexOf('/') != temppath.size() - 1)
                   {
                       columnData << QString::fromStdString(
                                        humanReadableBytes(
                                            fileListCopy.at(i).fileSize));
                   }

                   parents.at(lastidx)->appendChild(
                               new ContentTree(columnData,
                                               hash, parents.at(lastidx)));
                   parents <<  parents.at(lastidx)->child(
                                   parents.at(lastidx)->childCount()-1);
                   lastidx = -1;
               }
               else
               {
                   //if not a parent, get file size
                   if (temppath.lastIndexOf('/') != temppath.size() - 1)
                   {
                       columnData << QString::fromStdString(
                                        humanReadableBytes(
                                            fileListCopy.at(i).fileSize));
                   }

                   parents.last()->appendChild(
                               new ContentTree(columnData,
                                               hash, parents.last()));
                   parents <<  parents.last()->child(
                                   parents.last()->childCount()-1);
               }
            }
        }
    }
}

}
