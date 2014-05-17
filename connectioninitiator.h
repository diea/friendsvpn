#ifndef CONNECTIONINITIATOR_H
#define CONNECTIONINITIATOR_H

#include <QObject>
#include <QMutex>
#include "user.h"
#include "bonjoursql.h"

class ControlPlaneConnection;
class DataPlaneConnection;
class DataPlaneServer;
class DataPlaneClient;
class ControlPlaneClient;
class ControlPlaneServer;
/**
 * @brief The ConnectionInitiator class singleton class that is responsible to initialize
 * the control and data plane connections.
 */
class ConnectionInitiator : public QObject
{
    Q_OBJECT
private:
    BonjourSQL* qSql;

    QSslCertificate cert;
    QSslKey key;
    ControlPlaneServer* server;
    QThread dpServerThread;
    DataPlaneServer* dpServer;

    QList<ControlPlaneClient*> clients;
    QList<ControlPlaneConnection*> connections;

    QList<DataPlaneClient*> dpclients;
    QList<DataPlaneConnection*> dpConnections;

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
     * @brief getConnection will return the control planeconnection with uid "uid"
     * and will create one if one did not exist
     * Is protected with a Mutex
     * @param uid
     * @return the connection that has uid "uid"
     */
    ControlPlaneConnection* getConnection(QString uid);

    /**
     * @brief getDpConnection same as @see getConnection but for the dataplane
     * @param uid
     * @return
     */
    DataPlaneConnection* getDpConnection(QString uid);

    void removeConnection(ControlPlaneConnection* con);
    void removeConnection(DataPlaneConnection* con);

    /**
     * @brief getMyUid
     * @return the user's local uid
     */
    QString getMyUid();
signals:

public slots:

};

#endif // CONNECTIONINITIATOR_H
