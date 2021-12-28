#ifndef IRA2020_H
#define IRA2020_H

#include <QtWidgets>
#include <QHostAddress>

namespace Area3001
{
    class Ira2020 : public QObject
    {
        Q_OBJECT
    public:
        QString         mac_string;
        QHostAddress    ip;
    };
}

#endif // IRA2020_H
