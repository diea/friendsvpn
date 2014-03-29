#include "connectioninitiator.h"
ConnectionInitiator::ConnectionInitiator(BonjourSQL* qSql, QObject *parent) :
    QObject(parent)
{
    this->qSql = qSql;
}

void ConnectionInitiator::run() {

}
