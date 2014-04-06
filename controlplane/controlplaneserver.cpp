#include "controlplaneserver.h"
#include "connectioninitiator.h"

ControlPlaneServer::ControlPlaneServer(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    this->listenAdr = listenAdr;
    this->listenPort = listenPort;

    tcpSrv = new QTcpServer(this);

    cfg = QSslConfiguration();
    sslSockList = QList<SslSocket*>();

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
    SslSocket* sslSock = new SslSocket();

    sslSock->setSslConfiguration(cfg);
    sslSock->setSocketDescriptor(socket->socketDescriptor());
    connect(sslSock, SIGNAL(encrypted()), this, SLOT(sslSockReady()));
    connect(sslSock, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));
    // XXX ignore safety concerns about the self signed certificate...
    // connect(sslSock, SIGNAL(sslErrors(const QList<QSslError>&)), sslSock, SLOT(ignoreSslErrors()));
    sslSockList.append(sslSock);
    sslSock->startServerEncryption(); // XXX encrypted() never sent on linux ubuntu 12.04 & fedora.
}

void ControlPlaneServer::sslSockReady() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    connect(sslSock, SIGNAL(readyRead()), this, SLOT(sslSockReadyRead()));
    // send HELLO packet
    QString hello("Uid:" + init->getMyUid() + "\r\n");
    sslSock->write("HELLO \r\n");
    sslSock->write(hello.toLatin1().constData());
    sslSock->flush();
}

void ControlPlaneServer::sslSockError(const QList<QSslError>& errors) {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}


void ControlPlaneServer::sslSockReadyRead() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    if (!sslSock->isAssociated()) { // not associated with a ControlPlaneConnection
        const char* buf = sslSock->readLine();
        QString bufStr(buf);
        qDebug() << bufStr;
        if (bufStr.startsWith("HELLO")) {
            QString uidStr(sslSock->readLine());
            uidStr.chop(2); // drop \r\0
            qDebug() << uidStr.remove(0, 4);
            // drop the Uid: part with the .remove and get the CPConnection* correspoding to this UID
            qDebug() << "Going into init";
            ControlPlaneConnection* con =  init->getConnection(bufStr.remove(0, 4));
            con->addMode(Server_mode, sslSock); // add server mode
            sslSock->setControlPlaneConnection(con); // associate the sslSock with it
            qDebug() << "ssl Sock associated";;
        }
    } else { // socket is associated with controlplaneconnection
        qDebug() << "Server received data";
        sslSock->getControlPlaneConnection()->readBuffer(sslSock->readAll().data());
    }
}

void ControlPlaneServer::sslDisconnected() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    sslSockList.removeAll(sslSock);
    sslSock->getControlPlaneConnection()->removeMode(Server_mode);
    sslSock->deleteLater();
}

