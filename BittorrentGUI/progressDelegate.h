#ifndef PROGRESSDELEGATE_H
#define PROGRESSDELEGATE_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QApplication>

class progressDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    progressDelegate(QObject *parent = 0)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const
    {
        int progress = index.data().toInt();  // How do I access my models .progress_ property?

        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = progress;
        progressBarOption.text = QString::number(progress) + "%";
        progressBarOption.textVisible = true;

        QApplication::style()->drawControl(QStyle::CE_ProgressBar,
            &progressBarOption, painter);
    }

};

#endif // PROGRESSDELEGATE_H
