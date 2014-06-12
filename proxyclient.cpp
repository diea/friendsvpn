#include "proxyclient.h"
#include "unixsignalhandler.h"
#include "bonjour/bonjourdiscoverer.h"
#include "rawsockets.h"

ProxyClient::~ProxyClient()
{
    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    u->removeIp(listenIp);
}

ProxyClient::ProxyClient(QByteArray md5, QByteArray servermd5,  QString serversrcIp,
                         int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    listenIp = newIP();
    this->con = con;
    this->servermd5 = servermd5;
    this->serversrcIp = serversrcIp;

    serverRecord = BonjourDiscoverer::recordHashes.value(servermd5);
    if (!serverRecord) {
        // no more available
        qFatal("The record is no more available");
    }
    rawSocks = RawSockets::getInstance();
    connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

void ProxyClient::run() {
    run_pcap(serverRecord->ips.at(0).toUtf8().data());
    timer.start(TIMEOUT_DELAY);
}

void ProxyClient::sendBytes(char *buf, int len, QString) {
    timer.start(TIMEOUT_DELAY);
    // the srcPort is changed in the helper
    rawSocks->writeBytes(listenIp, serverRecord->ips.at(0), port, buf, sockType, len);
}

void ProxyClient::receiveBytes(const char* buf, int len, int sockType, QString) {
    con->sendBytes(buf, len, servermd5, sockType, serversrcIp);
}

void ProxyClient::timeout() {
    qDebug() << "Proxy client timeout";
    this->deleteLater(); // delete myself
}
