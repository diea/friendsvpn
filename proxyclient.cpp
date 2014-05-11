#include "proxyclient.h"
#include "unixsignalhandler.h"
#include "bonjour/bonjourdiscoverer.h"

ProxyClient::ProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
    serverRecord = BonjourDiscoverer::recordHashes.value(md5);
    if (!serverRecord) {
        // no more available
        qFatal("The record is no more available");
    }

    // I am a client so I know the dstIp is the server
    // I change the dstPort to the one from the server
    // I use listenPort which may be different than original port if bind has failed
    sendRaw = initRaw(serverRecord->ips.at(0), port);
}

void ProxyClient::run() {
    run_pcap();
    // TODO check when pcap listens on other port than original client port and change when sending TCP
}

void ProxyClient::sendBytes(const char *buf, int len, QString) {
    qDebug() << "I am a client sending bytes!" << buf;
    // dstIp argument is unused by client, it's for the server to know to which client to send

    // the srcPort is changed in the helper :)
    sendRaw->write(buf, len);
}

/*void ProxyClient::receiveBytes(char *buf, int len, int sizeLen, QString& hash, int sockType, QString& srcIp) {
    if (port != listenPort) {
        char* packet = buf + sizeLen;
        // first 16 bits = source Port of UDP and TCP
        qint16* dstPort = static_cast<qint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
        *dstPort = ntohs(listenPort); // restore the original client's port
    }

    con->sendBytes(buf, len, hash, sockType, srcIp);
}*/
