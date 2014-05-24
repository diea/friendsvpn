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

class ControlPlaneServer : public QObject
{
    Q_OBJECT
private:
    QHostAddress listenAdr;
    int listenPort;

    QTcpServer* tcpSrv;
    QList<SslSocket*> sslSockList;
    SSL_CTX *ctx;

    ConnectionInitiator* init;
public:
    explicit ControlPlaneServer(QSslCertificate servCert, QSslKey myKey,
                                QHostAddress listenAdr, int listenPort, QObject *parent = 0);
    ~ControlPlaneServer();
    // TODO destructor
signals:

public slots:
    /**
     * @brief start will start the server
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
