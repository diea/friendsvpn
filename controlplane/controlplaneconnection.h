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
    /**
     * These are the two SslSockets associated to this control plane connection
     */
    SslSocket* serverSock;
    SslSocket* clientSock;

    DatabaseHandler* qSql;

    /**
     * @brief mutex used to protect the current mode before sending
     */
    QMutex mutex;

    /**
     * @brief lastRcvdTimestamp contains the timestamp of the last received packet
     */
    uint lastRcvdTimestamp;

    /**
     * @brief proxyServers associates this control plane connection to proxy servers for each
     * of the received services from the distant host
     */
    QStack<ProxyServer*> proxyServers;

    /**
     * @brief removeConnection is called when the connected arrives in Both mode. The Facebook
     * UID's will be compared and the if we have a lower UID than the distant host we become "client",
     * otherwise we "server"
     */
    void removeConnection();

    /**
     * @brief sendPacket sends the message "pack" over the control plane connection
     * @param pack
     */
    void sendPacket(QString& pack);

    /**
     * As the control plane connection is a stream, an inputBuffer is used to buffer received
     * bytes until a full message has been received. (i.e. until two CR,LF sequences have been received)
     */
    char* inputBuffer;
    int bytesReceived;
    int lastFullPacket;
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
     * @brief readBuffer will read a buffer of lenght len received by the server or client
     */
    void readBuffer(char* buf, int len);
private slots:
    void wasDisconnected();

    /**
     * @brief aliveTimeout is called when the "alive" timer has timed out, this calls the abstract
     * "wasDisconnected" method.
     */
    void aliveTimeout();

public slots:
    /**
     * @brief The sendBonjour method will send the BONJOUR messages corresponding to the
     * currently active records which are authorized for the distant host.
     */
    void sendBonjour();
    void alive();
};

#endif // CONTROLPLANECONNECTION_H
