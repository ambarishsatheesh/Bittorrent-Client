#ifndef CONTENTTREE_H
#define CONTENTTREE_H

#include <QVector>
#include <QVariant>

class ContentTree
{
public:
    explicit ContentTree(const QList<QVariant> &data,
                         unsigned int id,
                         ContentTree *parentItem = nullptr);
    ~ContentTree();

    void appendChild(ContentTree *child);

    ContentTree *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    ContentTree *parentItem();
    unsigned int getIndex();

private:
    QVector<ContentTree*> m_childItems;
    QList<QVariant> m_itemData;
    ContentTree *m_parentItem;
    unsigned int id;
};

#endif // CONTENTTREE_H
