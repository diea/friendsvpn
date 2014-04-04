#include "threadedcpserver.h"

ThreadedCpServer::ThreadedCpServer(QSslCertificate servCert, QSslKey myKey,
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

void ThreadedCpServer::start() {
    tcpSrv->listen(listenAdr, listenPort);
    connect(tcpSrv, SIGNAL(newConnection()), this, SLOT(newIncoming()));
}

void ThreadedCpServer::newIncoming() {
    QTcpSocket* socket = tcpSrv->nextPendingConnection();
    SslServerThread* thread = new SslServerThread(socket->socketDescriptor(), cfg);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
