#include "proxyclient.h"
#include "unixsignalhandler.h"
#include "bonjour/bonjourdiscoverer.h"
#include "rawsockets.h"

ProxyClient::ProxyClient(QByteArray md5, QByteArray servermd5,  QString serversrcIp, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
    this->servermd5 = servermd5;
    this->serversrcIp = serversrcIp;

    serverRecord = BonjourDiscoverer::recordHashes.value(servermd5);
    if (!serverRecord) {
        // no more available
        qFatal("The record is no more available");
    }
    rawSocks = RawSockets::getInstance();
}

void ProxyClient::run() {
    run_pcap();

    // proxy clients timeout, connect QTimer
    //connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
    //timer.start(PROXYCLIENT_TIMEOUT); // timeout after 10seconds
}

void ProxyClient::sendBytes(const char *buf, int len, QString) {
    // dstIp argument is unused by client, it's for the server to know to which client to send

    // the srcPort is changed in the helper
    rawSocks->writeBytes(listenIp, serverRecord->ips.at(0), port, buf, sockType, len);
}

void ProxyClient::receiveBytes(const char* buf, int len, int sockType, QString&) {
    con->sendBytes(buf, len, servermd5, sockType, serversrcIp);
}

void ProxyClient::timeout() {
    this->deleteLater(); // delete myself
}
