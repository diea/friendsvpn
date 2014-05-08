#include "proxyclient.h"
#include "unixsignalhandler.h"

ProxyClient::ProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
}

void ProxyClient::run() {
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

    QProcess* bindSocket = new QProcess(this);
    QStringList bindSocketArgs;
    bindSocketArgs.append(QString::number(sockType));
    bindSocketArgs.append(QString::number(ipProto));
    bindSocketArgs.append(QString::number(listenPort));
    bindSocketArgs.append(listenIp);
    qDebug() << "bind socket args" << bindSocketArgs;
    bindSocket->start(QString(HELPERPATH) + "newSocket", bindSocketArgs);
    if (bindSocket->waitForFinished(200)) { // 200ms should be plenty enough for the process to fail on bind
        if (bindSocket->exitCode() == 3) {
            qWarning() << "Could not bind on port " << listenPort << " this client proxy will fail";
            bindSocket->deleteLater();
            // maybe send RST or ICMPv6 in this case; should happen VERY RARELY that a src port
            // was bound on all IPs...
            return;
        }
    }

    // listen for packets with pcap, forward on the secure UDP link
    QStringList args;
#ifdef __APPLE__
    args.append("lo0"); // TODO get default iface AND listen also on lo !
#elif __GNUC__
    args.append("lo"); // TODO get default iface AND listen also on lo !
#endif
    QString transportStr;
    sockType == SOCK_DGRAM ? transportStr = "udp" : transportStr = "tcp";
    args.append("ip6 dst host " + listenIp + " and " + transportStr + " and dst port " + QString::number(listenPort));
    QProcess* pcap = new QProcess(this);
    connect(pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));
    // TODO connect pcap to deleteLater ?
    connect(pcap, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
    qDebug() << args;

    u->addQProcess(pcap); // add pcap to be killed when main is killed

    pcap->start(QString(HELPERPATH) + "pcapListen", args);

}
