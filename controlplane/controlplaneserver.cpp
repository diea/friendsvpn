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

    init = ConnectionInitiator::getInstance();
}

ControlPlaneServer::~ControlPlaneServer()
{
    qDebug() << "Destroying control plane server";
    foreach (SslSocket* sock, sslSockList) {
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

    sslSockList.append(sslSock);
    sslSock->startServerEncryption(); // XXX encrypted() never sent on linux ubuntu 12.04 & fedora.
}

void ControlPlaneServer::sslSockReady() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());

    Q_IPV6ADDR addr = sslSock->peerAddress().toIPv6Address();
    char v6string[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(addr), v6string, INET6_ADDRSTRLEN);

    connect(sslSock, SIGNAL(readyRead()), this, SLOT(sslSockReadyRead()));

    // send HELLO packet
    sslSock->write("HELLO\r\n");
    QString hello("Uid:" + init->getMyUid() + "\r\n\r\n");
    sslSock->write(hello.toLatin1().constData());
    sslSock->flush();
}

void ControlPlaneServer::sslSockError(const QList<QSslError>& errors) {
    qDebug() << "SSL Error:";
    qDebug() << errors;
}

void ControlPlaneServer::sslSockReadyRead() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());

    if (!sslSock->isAssociated()) {
        // not associated
        QString msg = sslSock->read(7);
        if (msg == "HELLO\r\n") {
            QString bu = sslSock->readLine();
            QStringList uidList = bu.split(":");
            QString r = sslSock->read(2); // get '\r\n'
            if (uidList.length() == 2) {
                QString rcvdUid = uidList.at(1);
                rcvdUid.chop(2);
                if (r == "\r\n") {
                    DatabaseHandler* db = DatabaseHandler::getInstance();
                    db->addUidForIP(sslSock->peerAddress(), rcvdUid);
                    ControlPlaneConnection* con = init->getConnection(rcvdUid);
                    con->addMode(Receiving, sslSock);
                    sslSock->setControlPlaneConnection(con);
                }
            }
        } else {
            qWarning() << "Error did not receive HELLO first";
            sslSock->close();
            sslSock->deleteLater();
        }
    } else {
        QByteArray bytesBuf = sslSock->readAll();
        sslSock->getControlPlaneConnection()->readBuffer(bytesBuf.data(), bytesBuf.length());
    }
}

void ControlPlaneServer::sslDisconnected() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    sslSockList.removeAll(sslSock);
    if (sslSock->isAssociated())
        sslSock->getControlPlaneConnection()->removeMode(Receiving);
    sslSock->deleteLater();
}

