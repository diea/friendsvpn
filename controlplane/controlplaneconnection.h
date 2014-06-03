#ifndef CONTROLPLANECONNECTION_H
#define CONTROLPLANECONNECTION_H

#include <QObject>
#include "databasehandler.h"
#include "abstractplaneconnection.h"
#include "proxyserver.h"

class SslSocket;
/**
 * @brief The ControlPlaneConnection class extends the AbstractPlaneConnection and will handle
 * a ControlPlaneConnection with a friend as per PH2PHTP.
 */
class ControlPlaneConnection : public AbstractPlaneConnection
{
    Q_OBJECT
private:
    SslSocket* serverSock;
    SslSocket* clientSock;

    DatabaseHandler* qSql;

    /**
     * @brief mutex used to protect the current mode before sending!
     */
    QMutex mutex;

    /**
     * @brief lastRcvdTimestap contains the timestamp of the last received packet
     */
    uint lastRcvdTimestamp;

    /**
     * @brief proxyServers associates this control plane connection to proxy servers for each
     * of the received services from the distant host
     */
    QStack<ProxyServer*> proxyServers;

    void removeConnection();

    void sendPacket(QString& pack);

    char* inputBuffer;
    int bytesReceived;
    int lastFullPacket;

    bool gotHello; /* connection does not accept any packets until HELLO has been received */
public:
    explicit ControlPlaneConnection(QString uid, AbstractPlaneConnection *parent = 0);
    ~ControlPlaneConnection();

    /**
     * @brief addMode will add the mode to the connection client
     * @param mode must be either Server_mode or Client_mode
     * @param socket (must be of type QSslSocket)
     * returns false on error
     */
    bool addMode(plane_mode mode, QObject* socket);

    /**
     * @brief readBuffer will read a buffer received by the server or client
     * @param buf
     */
    void readBuffer(const char* buf, int len);
signals:
    /**
     * @brief uid_received emitted when the connection leaves the Wait_uid plane_state
     */
    void uid_received();
private slots:
    void wasDisconnected();
    void aliveTimeout();

public slots:
    void sendBonjour();
    void alive();
};

#endif // CONTROLPLANECONNECTION_H
