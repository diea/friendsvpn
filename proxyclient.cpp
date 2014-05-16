#include "proxyclient.h"
#include "unixsignalhandler.h"
#include "bonjour/bonjourdiscoverer.h"
#include "rawsockets.h"

ProxyClient::ProxyClient(QString md5, QString servermd5,  QString serversrcIp, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
    this->servermd5 = servermd5;
    this->serversrcIp = serversrcIp;

    qDebug() << "servermd5" << servermd5;
    qDebug() << BonjourDiscoverer::recordHashes;

    serverRecord = BonjourDiscoverer::recordHashes.value(servermd5);
    if (!serverRecord) {
        // no more available
        qFatal("The record is no more available");
    }
    rawSocks = RawSockets::getInstance();
}

void ProxyClient::run() {
    run_pcap();
    // I am a client so I know the dstIp is the server
    // I change the dstPort to the one from the server
    // I use listenPort which may be different than original port if bind has failed
}

void ProxyClient::sendBytes(const char *buf, int len, QString) {
    // dstIp argument is unused by client, it's for the server to know to which client to send

    qDebug() << "Client sending bytes to " << serverRecord->ips.at(0);
    // the srcPort is changed in the helper :)
    rawSocks->writeBytes(listenIp, serverRecord->ips.at(0), port, buf, sockType, len);
}

void ProxyClient::receiveBytes(const char* buf, int len, int sockType, QString& srcIp) {
    qDebug() << "client sending bytes with srcIp" << serversrcIp;
    con->sendBytes(buf, len, servermd5, sockType, serversrcIp);
}
