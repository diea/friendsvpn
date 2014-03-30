#include "connectioninitiator.h"
ConnectionInitiator::ConnectionInitiator(BonjourSQL* qSql, QObject *parent) :
    QObject(parent)
{
    this->qSql = qSql;
}

void ConnectionInitiator::run() {
    // start the server


    // start the clients
    /*QList< User* > friends = qSql->getFriends();

    foreach(User* frien_d, friends) {

    }*/

}

void ConnectionInitiator::startServer() {

}

void ConnectionInitiator::startClients() {

}
