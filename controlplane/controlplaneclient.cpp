#include "controlplaneclient.h"
#include "connectioninitiator.h"
ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    this->con = con;
    this->addr = addr;
    this->port = port;
    this->servCert = servCert;

    sslClient = new SslSocket();

    sslClient->addCaCertificate(servCert);
    sslClient->setPrivateKey(myKey);
    sslClient->setPeerVerifyName("facebookApp"); // common name for all our certificates

    init = ConnectionInitiator::getInstance();
}

ControlPlaneClient::~ControlPlaneClient()
{
    sslClient->close();
    delete sslClient;
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
    qDebug() << "Sending a write !!";
    // TODO protocol starts here
    // Send HELLO packet
    QString hello("HELLO\r\nUid:" + init->getMyUid() + "\r\n");
    sslClient->write(hello.toUtf8().constData());
}

void ControlPlaneClient::sslClientReadyRead() {
    if (!sslClient->isAssociated()) { // not associated with a ControlPlaneConnection
        const char* buf = sslClient->readAll();
        QString bufStr(buf);
        if (bufStr.startsWith("HELLO")) {
            qDebug() << bufStr.left(2).right(11);
            //ControlPlaneConnection* con =  init->getConnection(bufStr.left(2).right(11));
        }
    }
}

