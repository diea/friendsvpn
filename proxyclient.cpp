#include "proxyclient.h"
#include "unixsignalhandler.h"
#include "bonjour/bonjourdiscoverer.h"

ProxyClient::ProxyClient(QString md5, QString servermd5,  QString serversrcIp, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
    qDebug() << "md5" << servermd5;
    this->servermd5 = servermd5;
    this->serversrcIp = serversrcIp;
    qDebug() << BonjourDiscoverer::recordHashes;
    serverRecord = BonjourDiscoverer::recordHashes.value(servermd5);
    if (!serverRecord) {
        // no more available
        qFatal("The record is no more available");
    }
    sendRaw = NULL;
}

void ProxyClient::run() {
    mutex.lock();
    run_pcap();
    // I am a client so I know the dstIp is the server
    // I change the dstPort to the one from the server
    // I use listenPort which may be different than original port if bind has failed
    qDebug() << "initiation raw on source port " << port;
    sendRaw = initRaw(serverRecord->ips.at(0), port);
    mutex.unlock();
}

void ProxyClient::sendBytes(const char *buf, int len, QString) {
    mutex.lock();
    while (!sendRaw) {
        mutex.unlock();
        QThread::msleep(200);
        mutex.lock();
    }


    qDebug() << "I am a client sending bytes!" << buf;
    // dstIp argument is unused by client, it's for the server to know to which client to send

    // the srcPort is changed in the helper :)
    sendRaw->write(buf, len);

    QFile tcpPacket("sendBytesClientPacket");
    tcpPacket.open(QIODevice::WriteOnly);
    tcpPacket.write(buf, len);

    mutex.unlock();
}

void ProxyClient::receiveBytes(const char* buf, int len, int sockType, QString& srcIp) {
    con->sendBytes(buf, len, servermd5, sockType, serversrcIp);
}
