#ifndef CONTENTTREEMODEL_H
#define CONTENTTREEMODEL_H

#include "contenttree.h"

#include <QAbstractItemModel>

class ContentTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ContentTreeModel(const QVector<std::string>& data, QObject *parent = nullptr);
    ~ContentTreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;


private:
    void setupModelData(const QVector<std::string>& data,
                        ContentTree *parent);
      int findNode(unsigned int& hash,
                   const QList<ContentTree*>& tList);

    ContentTree *rootItem;
};

#endif // CONTENTTREEMODEL_H
