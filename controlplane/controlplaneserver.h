#ifndef CONTROLPLANESERVER_H
#define CONTROLPLANESERVER_H

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

class ControlPlaneServer : public QObject
{
    Q_OBJECT
private:
    QTcpServer* tcpSrv;
    QList<QSslSocket*> sslSockList;
    QSslConfiguration cfg;

public:
    explicit ControlPlaneServer(QHostAddress listenAdr, int listenPort, QObject *parent = 0);
    // TODO destructor
signals:

public slots:
    void newIncoming();
    void sslSockReady();
    void sslSockError(const QList<QSslError>& errors);
    void sslSockReadyRead();
};

#endif // CONTROLPLANECONNECTION_H
