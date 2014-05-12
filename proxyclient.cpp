#include "proxyclient.h"
#include "unixsignalhandler.h"
#include "bonjour/bonjourdiscoverer.h"

ProxyClient::ProxyClient(QString md5, QString servermd5, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
    qDebug() << "md5" << servermd5;
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
    if (!sendRaw) {
        mutex.unlock();
        return;
    }

    qDebug() << "I am a client sending bytes!" << buf;
    // dstIp argument is unused by client, it's for the server to know to which client to send

    // the srcPort is changed in the helper :)
    sendRaw->write(buf, len);
    mutex.unlock();
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
