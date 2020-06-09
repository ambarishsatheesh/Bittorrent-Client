#ifndef TORRENTHEADERCHECKBOX_H
#define TORRENTHEADERCHECKBOX_H

#include <QCheckBox>
#include <QVariant>

//Subclass of QCheckBox made to allow data to be sent to slots

class TorrentHeaderCheckbox : public QCheckBox
{
    Q_OBJECT

public:
    TorrentHeaderCheckbox(const QString &text, QWidget *parent = 0);

    void setData(const QVariant& userData);
    QVariant data() const;

private:
    QVariant storedData;
};

#endif // TORRENTHEADERCHECKBOX_H
