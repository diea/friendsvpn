#ifndef CONNECTIONINITIATOR_H
#define CONNECTIONINITIATOR_H

#include <QObject>
#include <QMutex>
#include "user.h"
#include "bonjoursql.h"
#include "threadedcpserver.h"
#include "controlplaneconnection.h"

class ControlPlaneClient;
class ControlPlaneServer;
/**
 * @brief The ConnectionInitiator class singleton class that is responsible to initialize
 * the control plane connections.
 */
class ConnectionInitiator : public QObject
{
    Q_OBJECT
private:
    BonjourSQL* qSql;

    QSslCertificate cert;
    QSslKey key;
    ControlPlaneServer* server;
    ThreadedCpServer* threadedServ;
    QList<ControlPlaneClient*> clients;
    QList<ControlPlaneConnection*> connections;

    static ConnectionInitiator* instance;
    /**
     * @brief startServer iniate an SSL server to receive connections from friends
     */
    void startServer();
    /**
     * @brief startClients initiate an SSL connection with each friend
     */
    void startClients();
    explicit ConnectionInitiator(QObject *parent = 0);

public:
    static ConnectionInitiator* getInstance();
    void run();

    /**
     * @brief getConnection will return the connection with uid "uid" and will create one if
     * one did not exist
     * Is protected with a Mutex
     * @param uid
     * @return the connection that has uid "uid"
     */
    ControlPlaneConnection* getConnection(QString uid);

    void removeConnection(ControlPlaneConnection* con);
    /**
     * @brief getMyUid
     * @return the user's local uid
     */
    QString getMyUid();
signals:

public slots:

};

#endif // CONNECTIONINITIATOR_H
