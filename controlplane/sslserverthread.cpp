#include "sslserverthread.h"

SslServerThread::SslServerThread(qintptr sockfd, QSslConfiguration sslConfig, QObject *parent) :
    QThread(parent), sockfd(sockfd), sslConfig(sslConfig)
{
}

void SslServerThread::run() {
    QSslSocket sslSock;
    sslSock.setSslConfiguration(sslConfig);
    sslSock.setSocketDescriptor(sockfd);

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
