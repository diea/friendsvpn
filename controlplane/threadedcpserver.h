#ifndef THREADEDCPSERVER_H
#define THREADEDCPSERVER_H
#include <QObject>
#include <QSslSocket>
#include <QTcpServer>
#include <QSocketNotifier>
#include <QHostAddress>
#include <QSslError>
#include <QList>
#include <QFile>
#include <QSslKey>
#include <QSslConfiguration>
#include <QSslCertificate>
#include "sslserverthread.h"

class ThreadedCpServer : public QObject
{
    Q_OBJECT
private:
    QHostAddress listenAdr;
    int listenPort;

    QTcpServer* tcpSrv;
    QList<QSslSocket*> sslSockList;
    QSslConfiguration cfg; // contains configuration used for each SSL connection

public:
    ThreadedCpServer(QSslCertificate servCert, QSslKey myKey,
                     QHostAddress listenAdr, int listenPort, QObject *parent);
    void start();

private slots:
    /**
     * @brief newIncoming is a slot for the newConnection() signal for the TcpServer.
     * This will launch a new SslSocket in a new thread.
     */
    void newIncoming();
};

#endif // THREADEDCPSERVER_H
