#ifndef CONNECTIONINITIATOR_H
#define CONNECTIONINITIATOR_H

#include <QObject>
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
    explicit ConnectionInitiator(BonjourSQL* qSql, QObject *parent = 0);

public:
    /**
     * @brief getInstance must be called to initialize the class
     * @param qSql
     * @return NULL on failure
     */
    static ConnectionInitiator* getInstance(BonjourSQL* qSql);
    /**
     * @brief getInstance can only be called if the class has already been initialized
     * @return NULL on failure
     */
    static ConnectionInitiator* getInstance();
    void run();

    void addControlPlaneConnection(ControlPlaneConnection* con);
    /**
     * @brief getConnection
     * @param uid
     * @return the connection that has uid "uid" and NULL if it doesn't exist
     */
    ControlPlaneConnection* getConnection(QString uid);

    /**
     * @brief getMyUid
     * @return the user's local uid
     */
    QString getMyUid();
signals:

public slots:

};

#endif // CONNECTIONINITIATOR_H
