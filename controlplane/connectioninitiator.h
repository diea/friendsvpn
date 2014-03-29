#ifndef CONNECTIONINITIATOR_H
#define CONNECTIONINITIATOR_H

#include <QObject>
#include "bonjoursql.h"

class ConnectionInitiator : public QObject
{
    Q_OBJECT
private:
    BonjourSQL* qSql;
public:
    explicit ConnectionInitiator(BonjourSQL* qSql, QObject *parent = 0);
    void run();
signals:

public slots:

};

#endif // CONNECTIONINITIATOR_H
