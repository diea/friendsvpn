#include "controlplaneserver.h"

ControlPlaneServer::ControlPlaneServer(QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    // create listen socket
    tcpSrv = new QTcpServer(this);
    // get my ip from SQL here

    tcpSrv->listen(listenAdr, listenPort);
    connect(tcpSrv, SIGNAL(newConnection()), this, SLOT(newIncoming()));

    sslSrv = new QSslSocket(this);
    QFile certFile("/Users/diea/tests/Qtssl/servCert.pem");
    if (!certFile.open(QIODevice::ReadOnly))
        qDebug() << "Could not open cert file";
    QSslCertificate cert(certFile.readAll(), QSsl::Pem);
    certFile.close();
    sslSrv->setLocalCertificate(cert);

    QFile keyFile("/Users/diea/tests/Qtssl/servKey.pem");
    if (!keyFile.open(QIODevice::ReadOnly))
        qDebug() << "cloud not open key file";
    QSslKey ssl_key(keyFile.readAll(), QSsl::Rsa);
    keyFile.close();
    sslSrv->setPrivateKey(ssl_key);

    QSslConfiguration cfg = sslSrv->sslConfiguration();
    cfg.caCertificates();
}

void ControlPlaneServer::newIncoming() {
    socket = tcpSrv->nextPendingConnection();
    qDebug() << "Incoming !" << socket->state();

    sslSrv->setSocketDescriptor(socket->socketDescriptor());
    //sslSrv->setProtocol(QSsl::Rsa);

    connect(sslSrv, SIGNAL(encrypted()), this, SLOT(sslSrvReady()));
    //connect(sslSrv, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(sslSrvError(const QList<QSslError>&)));
    connect(sslSrv, SIGNAL(sslErrors(const QList<QSslError>&)), sslSrv, SLOT(ignoreSslErrors()));


    sslSrv->startServerEncryption();
    qDebug() << "after incoming !" << socket->state();
    //qDebug() << sslSrv->waitForEncrypted();
}

void ControlPlaneServer::sslSrvReady() {
    qDebug() << "encrypted!" << sslSrv->state();
    connect(sslSrv, SIGNAL(readyRead()), this, SLOT(sslSrvReadyRead()));

    //sslNotifier = new QSocketNotifier(sslSrv->socketDescriptor(), QSocketNotifier::Read, this); <= GRÔÔSE ERREUR ! :D => BLOG !
    //connect(sslNotifier, SIGNAL(activated(int)), this, SLOT(sslSrvReadyRead()));
}

void ControlPlaneServer::sslSrvError(const QList<QSslError>& errors) {
    qDebug() << "ssl error";
    qDebug() << errors;
}


void ControlPlaneServer::sslSrvReadyRead() {
    qDebug() << "Read ready";
    /*char buf[2048];
    qint64 lineLength = sslSrv->readLine(buf, sizeof(buf));
    if (lineLength > 0) {
        QString buff (buf);
        buff.truncate(lineLength);
        qDebug() << buff;
    }*/
    qDebug() << sslSrv->readAll().data();
    sslSrv->write("He ouais ça marche :)");
    //qDebug() << socket->readAll().data();
    //sslSrv->close();
}

