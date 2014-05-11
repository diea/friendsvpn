#include "proxyclient.h"
#include "unixsignalhandler.h"

ProxyClient::ProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
}

void ProxyClient::run() {
    run_pcap();
    // TODO check when pcap listens on other port than original client port and change when sending TCP
}
