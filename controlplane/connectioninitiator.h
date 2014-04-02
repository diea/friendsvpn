#ifndef CONNECTIONINITIATOR_H
#define CONNECTIONINITIATOR_H

#include <QObject>
#include "user.h"
#include "bonjoursql.h"
#include "controlplaneserver.h"
#include "controlplaneclient.h"

class ConnectionInitiator : public QObject
{
    Q_OBJECT
private:
    BonjourSQL* qSql;

    QSslCertificate cert;
    QSslKey key;
    ControlPlaneServer* server;
    QList<ControlPlaneClient*> clients;

    /**
     * @brief startServer iniate an SSL server to receive connections from friends
     */
    void startServer();
    /**
     * @brief startClients initiate an SSL connection with each friend
     */
    void startClients();

public:
    explicit ConnectionInitiator(BonjourSQL* qSql, QObject *parent = 0);
    void run();
signals:

public slots:

};

#endif // CONNECTIONINITIATOR_H
