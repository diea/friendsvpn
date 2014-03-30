#include "controlplaneserver.h"

ControlPlaneServer::ControlPlaneServer(QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    // create listen socket
    tcpSrv = new QTcpServer(this);
    // get my ip from SQL here

    tcpSrv->listen(listenAdr, listenPort);
    connect(tcpSrv, SIGNAL(newConnection()), this, SLOT(newIncoming()));
    cfg = QSslConfiguration();

    sslSockList = QList<QSslSocket*>();
    QFile certFile("/Users/diea/tests/Qtssl/servCert.pem");
    if (!certFile.open(QIODevice::ReadOnly))
        qDebug() << "Could not open cert file";
    QSslCertificate cert(certFile.readAll(), QSsl::Pem);
    certFile.close();
    cfg.setLocalCertificate(cert);

    QFile keyFile("/Users/diea/tests/Qtssl/servKey.pem");
    if (!keyFile.open(QIODevice::ReadOnly))
        qDebug() << "cloud not open key file";
    QSslKey ssl_key(keyFile.readAll(), QSsl::Rsa);
    keyFile.close();
    cfg.setPrivateKey(ssl_key);
}

void ControlPlaneServer::newIncoming() {
    QTcpSocket* socket = tcpSrv->nextPendingConnection();
    qDebug() << "Incoming !" << socket->state();
    QSslSocket* sslSock = new QSslSocket(this);
    sslSock->setSslConfiguration(cfg);
    sslSock->setSocketDescriptor(socket->socketDescriptor());

    connect(sslSock, SIGNAL(encrypted()), this, SLOT(sslSockReady()));
    // XXX ignore safety concerns about the self signed certificate...
    connect(sslSock, SIGNAL(sslErrors(const QList<QSslError>&)), sslSock, SLOT(ignoreSslErrors()));
    sslSockList.append(sslSock);
    sslSock->startServerEncryption();
    qDebug() << "after incoming !" << socket->state();
}

void ControlPlaneServer::sslSockReady() {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    qDebug() << "encrypted!" << sslSock->state();
    connect(sslSock, SIGNAL(readyRead()), this, SLOT(sslSockReadyRead()));
}

void ControlPlaneServer::sslSockError(const QList<QSslError>& errors) {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}


void ControlPlaneServer::sslSockReadyRead() {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    qDebug() << "Read ready";
    char buf[2048];
    qint64 lineLength = sslSock->readLine(buf, sizeof(buf));
    if (lineLength > 0) {
        QString buff (buf);
        buff.truncate(lineLength);
        qDebug() << buff;
    }
    qDebug() << sslSock->readAll().data();
    sslSock->write("He ouais ça marche :)");
}

