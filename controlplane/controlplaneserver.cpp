#include "controlplaneserver.h"

ControlPlaneServer::ControlPlaneServer(QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    this->listenAdr = listenAdr;
    this->listenPort = listenPort;

    tcpSrv = new QTcpServer(this);

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
    qDebug() << "ControlPlaneServer Thread : " << QThread::currentThreadId();
    tcpSrv->listen(listenAdr, listenPort);
    connect(tcpSrv, SIGNAL(newConnection()), this, SLOT(newIncoming()));
}

void ControlPlaneServer::newIncoming() {
    qDebug() << "Incoming Thread : " << QThread::currentThreadId();
    QTcpSocket* socket = tcpSrv->nextPendingConnection();
    qDebug() << "Incoming !" << socket->state();

    QSslSocket* sslSock = new QSslSocket(this);
    sslSock->setSslConfiguration(cfg);
     qDebug() << "NEW SOCKET";
    sslSock->setSocketDescriptor(socket->socketDescriptor());
     qDebug() << "NEW SOCKET";
    connect(sslSock, SIGNAL(encrypted()), this, SLOT(sslSockReady()));
    connect(sslSock, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));
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

void ControlPlaneServer::sslDisconnected() {
    QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    if (!sslSock)
        return;
    sslSockList.removeAll(sslSock);
    sslSock->deleteLater();
}

