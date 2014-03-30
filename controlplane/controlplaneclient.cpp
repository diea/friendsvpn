#include "controlplaneclient.h"
ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    sslClient = new QSslSocket();
    sslClient->addCaCertificate(servCert);
    sslClient->setPrivateKey(myKey);
}

ControlPlaneClient::~ControlPlaneClient()
{
    sslClient->close();
    delete sslClient;
}

void ControlPlaneClient::run() {
    // ignore errors since we are using self-signed certificates
    connect(sslClient, SIGNAL(sslErrors(const QList<QSslError>&)), sslClient, SLOT(ignoreSslErrors()));
    connect(sslClient, SIGNAL(encrypted()), this, SLOT(connectionReady()));

    sslClient->connectToHostEncrypted(addr, port);
}

void ControlPlaneClient::connectionReady() {
    // TODO protocol starts here
    sslClient->write("Hello world");
}

