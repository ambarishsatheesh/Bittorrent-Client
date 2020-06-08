#ifndef STOREDPREVTORRENTS_H
#define STOREDPREVTORRENTS_H

#endif // STOREDPREVTORRENTS_H

#include <QList>
#include <QString>

struct storedPrevTorrents
{
    QList<QString> torrentAddedOn;
    QList<QString> torrentName;
    QList<long long> torrentSize;

};
