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
    QSslSocket* sslSrv;
    QTcpSocket* socket;
    QSocketNotifier* sslNotifier;
public:
    explicit ControlPlaneServer(QHostAddress listenAdr, int listenPort, QObject *parent = 0);
    // TODO destructor
signals:

public slots:
    void newIncoming();
    void sslSrvReady();
    void sslSrvError(const QList<QSslError>& errors);
    void sslSrvReadyRead();
};

#endif // CONTROLPLANECONNECTION_H
