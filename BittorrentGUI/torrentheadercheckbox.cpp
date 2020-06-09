#include "torrentheadercheckbox.h"

TorrentHeaderCheckbox::TorrentHeaderCheckbox(
        const QString &text, QWidget *parent)
    : QCheckBox(parent)
{
    setText(text);
}

void TorrentHeaderCheckbox::setData(const QVariant& userData)
{
    storedData = userData;
}

QVariant TorrentHeaderCheckbox::data() const
{
    return storedData;
}


