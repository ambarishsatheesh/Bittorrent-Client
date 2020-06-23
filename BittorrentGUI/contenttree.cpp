#include "contenttree.h"


ContentTree::ContentTree(const QList<QVariant> &data,
                         unsigned int id,
                         ContentTree *parent)
    : m_itemData(data), m_parentItem(parent), id{id}
{
}


ContentTree::~ContentTree()
{
    qDeleteAll(m_childItems);
}

void ContentTree::appendChild(ContentTree *item)
{
    m_childItems.append(item);
}

ContentTree *ContentTree::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int ContentTree::childCount() const
{
    return m_childItems.count();
}

int ContentTree::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<ContentTree*>(this));

    return 0;
}

int ContentTree::columnCount() const
{
    return m_itemData.count();
}

QVariant ContentTree::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QVariant();
    return m_itemData.at(column);
}

ContentTree *ContentTree::parentItem()
{
    return m_parentItem;
}


unsigned int ContentTree::getIndex()
{
    return id;
}

