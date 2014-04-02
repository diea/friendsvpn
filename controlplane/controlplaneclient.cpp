#include "controlplaneclient.h"
ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    this->addr = addr;
    this->port = port;
    this->servCert = servCert;

    sslClient = new QSslSocket();

    sslClient->addCaCertificate(servCert);
    sslClient->setPrivateKey(myKey);
    sslClient->setPeerVerifyName("facebookApp"); // common name for all our certificates
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
    // TODO protocol starts here
    sslClient->write("Hello world");
}

