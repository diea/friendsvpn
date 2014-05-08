#include "proxyclient.h"
#include "unixsignalhandler.h"

ProxyClient::ProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con) :
    Proxy(srcPort, sockType, md5)
{
    this->con = con;
}

/*ProxyClient* ProxyClient::getProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con) {
    if (proxyHashes.contains(md5)) {
        return proxyHashes.value(md5);
    } else {
        return new ProxyClient(md5, sockType, srcPort, con);
    }
}*/

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
void ProxyClient::readyRead() {
    QProcess* pcap = dynamic_cast<QProcess*>(QObject::sender());
    if (left == 0) {
        // get length
        char c;
        pcap->getChar(&c);
        if (c != '[') {
            qWarning() << "format error";
            return;
        }
        QString nb;

        pcap->getChar(&c);
        while (c != ']') {
            if (!isdigit(c)) {
                qWarning() << "format error, digit required between []";
                return;
            }
            nb.append(c);
            pcap->getChar(&c);
        }

        if (nb.isEmpty()) { qWarning() << "format error, empty packet length!"; return; }

        left = nb.toInt();
    }

    if (pcap->bytesAvailable() < left) {
        qDebug() << "waiting for more bytes";
        return; // wait for more bytes
    }

    // get packet and send it to dtls connection
    char packet[2000];
    pcap->read(packet, left);
    qDebug() << "Read" << left << "bytes";

    // send over DTLS with friendUid
    con->sendBytes(packet, left, idHash, sockType);

    left = 0;
}

void ProxyClient::pcapFinish(int exitCode) {
    qWarning() << "pcap exited with exit code " << exitCode << "for client" << listenIp << listenPort;
}

/*void ProxyClient::sendBytes(const char *buf, int len) {
    // use raw helper to send buffer
}*/
