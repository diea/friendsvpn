#include "controlplaneclient.h"
#include "connectioninitiator.h"
ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QString friendUid, QObject *parent) :
    QObject(parent)
{
    this->addr = addr;
    this->port = port;
    this->servCert = servCert;
    this->friendUid = friendUid;
    sslClient = new SslSocket();

    sslClient->addCaCertificate(servCert);
    sslClient->setPrivateKey(myKey);
    sslClient->setPeerVerifyName("facebookApp"); // common name for all our certificates

    init = ConnectionInitiator::getInstance();
}

ControlPlaneClient::~ControlPlaneClient()
{
    qDebug() << "Destroy contorl plane client";
    sslClient->close();
    //delete sslClient;
}

void ControlPlaneClient::run() {
    if (servCert.isNull()) {
        qWarning() << "Warning, host " << this->addr << " has not set a certificate for SSL dataplane "
                    "connection";
        return; // do nothing if serv cert was not set!
    }

    // ignore errors since we are using self-signed certificates
    // connect(sslClient, SIGNAL(sslErrors(const QList<QSslError>&)), sslClient, SLOT(ignoreSslErrors()));
    connect(sslClient, SIGNAL(sslErrors(const QList<QSslError>&)), this,
            SLOT(sslErrors(const QList<QSslError>&)));
    connect(sslClient, SIGNAL(encrypted()), this, SLOT(connectionReady()));
    connect(sslClient, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));

    qDebug() << "CONNECT TO HOST " << addr.toString() << ":" << port;
    sslClient->connectToHostEncrypted(addr.toString(), port);
}

void ControlPlaneClient::sslErrors(const QList<QSslError>& errors) {
    //QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}

void ControlPlaneClient::connectionReady() {
    connect(sslClient, SIGNAL(readyRead()), this, SLOT(sslClientReadyRead()));
    // Send HELLO packet
    sslClient->setControlPlaneConnection(init->getConnection(friendUid));
    QString hello("HELLO\r\nUid:" + init->getMyUid() + "\r\n\r\n");
    sslClient->write(hello.toUtf8().constData());
    sslClient->flush();
}

void ControlPlaneClient::sslClientReadyRead() {
    QByteArray bytesBuf = sslClient->readAll();
    sslClient->getControlPlaneConnection()->readBuffer(bytesBuf.data(), bytesBuf.length());
}

void ControlPlaneClient::sslDisconnected() {
    qDebug() << "Disco";
    if (sslClient->isAssociated())
        sslClient->getControlPlaneConnection()->removeMode(Emitting);
    sslClient->deleteLater();
}

