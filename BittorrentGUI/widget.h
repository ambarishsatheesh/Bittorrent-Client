#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QTableView;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

public slots:
    void customHeaderMenuRequested(QPoint pos);

private:
    QTableView *table;
};

#endif // WIDGET_H
