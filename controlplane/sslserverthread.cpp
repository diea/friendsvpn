#include "sslserverthread.h"

SslServerThread::SslServerThread(qintptr sockfd, QSslConfiguration sslConfig, QObject *parent) :
    QThread(parent), sockfd(sockfd), sslConfig(sslConfig)
{
}

void SslServerThread::run() {
    QTcpSocket tcpSock;
    if (!tcpSock.setSocketDescriptor(sockfd)) {
        qDebug() << tcpSock.errorString();
        return;
    }
    QSslSocket sslSock;
    sslSock.setSslConfiguration(sslConfig);
    sslSock.setSocketDescriptor(tcpSock.socketDescriptor());

    sslSock.startServerEncryption();

    if (!sslSock.waitForEncrypted()) {
        qDebug() << "server failed";
        qDebug() << sslSock.errorString();
    } else {
        qDebug() << "server reading ! ";
        while (sslSock.waitForReadyRead())
            qDebug() << sslSock.readAll().data();
    }

    sslSock.close();
}
