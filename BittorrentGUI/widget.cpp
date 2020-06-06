#include "widget.h"

#include <QVBoxLayout>
#include <QTableView>
#include <QMenu>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *l=new QVBoxLayout(this);
    table=new QTableView(this);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(customMenuRequested(QPoint)));
    l->addWidget(table);
}

Widget::~Widget(){}

void Widget::customHeaderMenuRequested(QPoint pos)
{
    QModelIndex index=table->indexAt(pos);

    QMenu *menu=new QMenu(this);
    menu->addAction(new QAction("Action 1", this));
    menu->addAction(new QAction("Action 2", this));
    menu->addAction(new QAction("Action 3", this));
    menu->popup(table->viewport()->mapToGlobal(pos));
}
