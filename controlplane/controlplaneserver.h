#ifndef CONTROLPLANESERVER_H
#define CONTROLPLANESERVER_H
#include <QObject>
#include <QTcpServer>
#include <QSocketNotifier>
#include <QHostAddress>
#include <QSslError>
#include <QList>
#include <QFile>
#include <QSslKey>
#include <QSslConfiguration>
#include <QSslCertificate>
#include "sslsocket.h"
#include "controlplaneconnection.h"

class ConnectionInitiator;
/**
 * @brief The ControlPlaneServer class receives incoming control plane connections. It will instantiate
 * a new SslSocket for each incoming connection.
 */
class ControlPlaneServer : public QObject
{
    Q_OBJECT
private:
    QHostAddress listenAdr;
    int listenPort;

    QTcpServer* tcpSrv;
    QList<SslSocket*> sslSockList;
    QSslConfiguration cfg; // contains configuration used for each SSL connection

    ConnectionInitiator* init;
public:
    explicit ControlPlaneServer(QSslCertificate servCert, QSslKey myKey,
                                QHostAddress listenAdr, int listenPort, QObject *parent = 0);
    ~ControlPlaneServer();
signals:

public slots:
    /**
     * @brief start will start the server (hint: use this to start into new QThread)
     */
    void start();
private slots:
    /**
     * @brief sslSockError called if there is an error with an SSL socket
     * @param errors
     */
    void sslSockError(const QList<QSslError>& errors);
    /**
     * @brief newIncoming used by the TcpServer to initiate a new SSL connection after TCP connection
     * has been established.
     */
    void newIncoming();
    /**
     * @brief sslSockReady called when the SSL connection is ready
     */
    void sslSockReady();
    /**
     * @brief sslSockReadyRead called when data has arrived on an SSL socket
     */
    void sslSockReadyRead();
    /**
     * @brief sslDisconnected called when an SSL socket has been disconnected
     */
    void sslDisconnected();
};

#endif // CONTROLPLANECONNECTION_H
