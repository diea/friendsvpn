#include "controlplaneserver.h"

ControlPlaneServer::ControlPlaneServer(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    this->listenAdr = listenAdr;
    this->listenPort = listenPort;

    tcpSrv = new QTcpServer(this);

    cfg = QSslConfiguration();
    sslSockList = QList<QSslSocket*>();

    cfg.setLocalCertificate(servCert);
    cfg.setPrivateKey(myKey);
}

ControlPlaneServer::~ControlPlaneServer()
{
    foreach (QSslSocket* sock, sslSockList) {
        sock->close();
        delete sock;
    }

    tcpSrv->close();
    delete tcpSrv;
}

void ControlPlaneServer::start() {
    tcpSrv->listen(listenAdr, listenPort);
    connect(tcpSrv, SIGNAL(newConnection()), this, SLOT(newIncoming()));
}

void ControlPlaneServer::newIncoming() {
    QTcpSocket* socket = tcpSrv->nextPendingConnection();
    QSslSocket* sslSock = new QSslSocket();

    sslSock->setSslConfiguration(cfg);
    sslSock->setSocketDescriptor(socket->socketDescriptor());
    connect(sslSock, SIGNAL(encrypted()), this, SLOT(sslSockReady()));
    connect(sslSock, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));
    // XXX ignore safety concerns about the self signed certificate...
    connect(sslSock, SIGNAL(sslErrors(const QList<QSslError>&)), sslSock, SLOT(ignoreSslErrors()));
    sslSockList.append(sslSock);
    sslSock->startServerEncryption();
}

void ControlPlaneServer::sslSockReady() {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    connect(sslSock, SIGNAL(readyRead()), this, SLOT(sslSockReadyRead()));
}

void ControlPlaneServer::sslSockError(const QList<QSslError>& errors) {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}


void ControlPlaneServer::sslSockReadyRead() {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());

    qDebug() << sslSock->readAll().data();
    sslSock->write("ça marche :)");
}

void ControlPlaneServer::sslDisconnected() {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    if (!sslSock)
        return;
    sslSockList.removeAll(sslSock);
    sslSock->deleteLater();
}

