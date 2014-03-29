#include "controlplaneclient.h"
#include <QThread>
ControlPlaneClient::ControlPlaneClient(QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    sslClient = new QSslSocket();
}

void ControlPlaneClient::run() {
    sleep(2);
    qDebug() << QThread::currentThreadId();
    if (!sslClient->waitForEncrypted()) {
        qDebug() << sslClient->errorString();
    } else {
        sslClient->write("GET / HTTP/1.0\r\n\r\n");
        while (sslClient->waitForReadyRead())
            qDebug() << sslClient->readAll().data();
    }
}

ControlPlaneClient::~ControlPlaneClient() {
    sslClient->close();
    delete sslClient;
}
