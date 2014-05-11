#include "proxy.h"
#include "unixsignalhandler.h"
#include <QCryptographicHash>
#include <string.h>

QHash<QString, Proxy*> Proxy::proxyHashes;
IpResolver* Proxy::resolver = IpResolver::getInstance();

QString Proxy::defaultIface = Proxy::getDefaultInterface();

Proxy::Proxy(int srcPort, int sockType, QString md5)
    : listenPort(srcPort), sockType(sockType)
{
    if (proxyHashes.contains(md5)) {
        throw 1; // already exists, we throw int "1"
    }
    idHash = md5;
    proxyHashes.insert(md5, this);
    left = 0;

    listenIp = newIP();

    if (sockType == SOCK_STREAM) ipProto = IPPROTO_TCP;
    else ipProto = IPPROTO_UDP;
}

Proxy::Proxy(int srcPort, const QString& regType, QString md5) : listenPort(srcPort)
{
    if (proxyHashes.contains(md5)) {
        throw 1; // already exists, we throw int "1"
    }
    idHash = md5;
    proxyHashes.insert(md5, this);
    left = 0;
    listenIp = newIP();
    if (regType.contains("tcp")) {
        sockType = SOCK_STREAM;
        ipProto = IPPROTO_TCP;
    } else {
        sockType = SOCK_DGRAM;
        ipProto = IPPROTO_UDP;
    }
}

QProcess* Proxy::initRaw(QString ipDst, int srcPort) {
    qDebug() << "initRaw";
    // init raw socket to send packets
    QProcess* sendRaw = new QProcess(this);
    QStringList sendRawArgs;
    IpResolver* resolv = IpResolver::getInstance();
    struct ip_mac_mapping map = resolv->getMapping(ipDst);
    sendRawArgs.append(map.interface);
    sendRawArgs.append(listenIp);
    sendRawArgs.append(ipDst);
    sendRawArgs.append(QString::number(sockType));
    sendRawArgs.append(QString::number(srcPort));
    //sendRawArgs.append(QString::number(dstPort));

    qDebug() << "sendRawArgs" << sendRawArgs;

    if (!map.mac.isEmpty()) {
        sendRawArgs.append(map.mac);
    }

    connect(sendRaw, SIGNAL(finished(int)), this, SLOT(sendRawFinish(int)));
    connect(sendRaw, SIGNAL(readyReadStandardError()), this, SLOT(sendRawStandardError()));
    connect(sendRaw, SIGNAL(readyReadStandardOutput()), this, SLOT(sendRawStandardOutput()));

    /*UnixSignalHandler* u = UnixSignalHandler::getInstance();
    u->addQProcess(sendRaw);*/ // TODO

    sendRaw->start(QString(HELPERPATH) + "sendRaw", sendRawArgs);
    qDebug() << "start raw!";
    return sendRaw;
}

QString Proxy::newIP() {
    bool newIp = true; // the new IP has not been assigned to iface yet or is not a duplicate
    QString newip;
    do {
        // random ULA
        newip = randomIP();
        // check it doesn't exist
        QProcess testIfconfig;

        testIfconfig.start("/sbin/ifconfig", QStringList(defaultIface));
        testIfconfig.waitForReadyRead();
        char buf[3000];
        int length;
        while ((length = testIfconfig.readLine(buf, 3000))) {
            QString curLine(buf);
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            foreach (QString value, list) {
                if (value.contains(newip)) {
                    newIp = false;
                    qDebug() << "local duplicate!";
                }
            }
        }
        testIfconfig.close();
    } while (!newIp);

    //qDebug() << "add with ifconfig!";
    // add it with ifconfig
    QProcess ifconfig;
    // TODO include in the app bundle to launch from there
    // TODO check DAD
    QStringList newipArgs;
    newipArgs.append(defaultIface);
    newipArgs.append(newip);
    ifconfig.start(QString(HELPERPATH) + "ifconfighelp", newipArgs);
    ifconfig.waitForReadyRead();
    qDebug() << ifconfig.readAllStandardOutput();
    qDebug() << ifconfig.readAllStandardError();
    ifconfig.close();

    newip.truncate(newip.length() - 3); // remove prefix

    // add to local cache!
    IpResolver* r = IpResolver::getInstance();

    // XXX better way of determining local loopback interface ?
#ifdef __APPLE__
    r->addMapping(newip, "", "lo0");
#elif __GNUC__
    r->addMapping(newip, "", "lo");
#endif
    return newip;
}


QString Proxy::getDefaultInterface() {
#ifdef __APPLE__
    QProcess netstat;
    netstat.start("netstat -nr");
    netstat.waitForReadyRead();
    char buf[3000];
    int length;
    while ((length = netstat.readLine(buf, 3000))) {
        buf[length - 1] = '\0'; // remove "\n"
        QString curLine(buf);
        if (curLine.startsWith("default")) {
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            if (list.at(1).contains(":")) {
                netstat.close();
                qDebug() << list.at(3);
                return list.at(3);
            }
        }
    }
    netstat.close();
    qWarning() << "The host has no IPv6 default route!";
    exit(-1);
#elif __GNUC__
    QProcess route;
    route.start("route -n6");
    route.waitForReadyRead();
    char buf[3000];
    int length;
    while ((length = route.readLine(buf, 3000))) {
        buf[length - 1] = '\0'; // remove "\n"
        QString curLine(buf);
        if (curLine.startsWith("::/0")) {
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            if (list.at(2) != "::") {
                route.close();
                qDebug() << list.at(6);
                return list.at(6);
            }
        }
    }
    route.close();
    qWarning() << "The host has no IPv6 default route!";
    exit(-1);
#endif
}

/**
 * undefined if i >= 16
 */
char Proxy::intToHexChar(int i) {
    if (i < 10)
        return (char)(((int)'0')+i);
    switch (i) {
        case 10: return 'a';
        case 11: return 'b';
        case 12: return 'c';
        case 13: return 'd';
        case 14: return 'e';
        case 15: return 'f';
        default: return 'a';
    }
}

struct prefix Proxy::getPrefix() {
    QProcess ifconfig;
    ifconfig.start("/sbin/ifconfig", QStringList(defaultIface));
    ifconfig.waitForReadyRead();
    char buf[3000];
    int length;
    while ((length = ifconfig.readLine(buf, 3000))) {
        QString curLine(buf);
        QStringList list = curLine.split(" ", QString::SkipEmptyParts);

        if (list.at(0).contains("inet6")) {
#ifdef __APPLE__
            if (!list.at(1).startsWith("fe80") && list.at(1).startsWith("fd3b")) { // remove fd3b & maybe remove ULA.
                QString ip = list.at(1);
                ifconfig.close();

                struct prefix p;
                p.str = stripIp(ip, list.at(3).toInt());
                p.len = list.at(3).toInt();
                return p;
            }
#elif __GNUC__
            if (!list.at(2).startsWith("fe80") && list.at(2).startsWith("fd3b")) {
                QString ip = list.at(2);
                int slashIndex = ip.indexOf("/");
                QString prefixLength = ip.right(ip.length() - slashIndex - 1);
                ip.truncate(slashIndex);
                ifconfig.close();

                struct prefix p;
                p.str = stripIp(ip, prefixLength.toInt());
                p.len = prefixLength.toInt();
                return p;
            }
#endif
        }
    }
    ifconfig.close();
    struct prefix p; // prefix of len 0 => invalid.
    p.len = 0;
    return p;
}

QString Proxy::stripIp(QString ip, int prefix) {
    /* convert to network format to clear bytes out of prefix */
    struct in6_addr ipv6network;
    inet_pton(AF_INET6, ip.toUtf8().data(), &ipv6network);

    int left = 128 - prefix;
    for (int i = 127; i >= 0 && left > 0; i--) { // 16 bytes in ipv6network
        unsigned char mask = (~(1 << i % 8));

        // go to host order
        unsigned char hostByte = 0;
        for (int k = 0; k < 8; k++) {
            hostByte |= ((ipv6network.s6_addr[i / 8] >> k) & 1) << (7-k);
        }
        hostByte &= mask;

        // go back to network byte order
        unsigned char newVal = 0;
        for (int k = 0; k < 8; k++) {
            newVal |= ((hostByte >> k) & 1) << (7-k);
        }

        ipv6network.s6_addr[i / 8] = newVal;
        left--;
    }


    char newIp[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ipv6network, newIp, sizeof(newIp));
    QString toReturn(newIp);

    // remove the last ":" since we just want the prefix and not the whole IP
    if (toReturn.count(":") > 4) toReturn.truncate(toReturn.length() - 1);

    return toReturn;
}

QString Proxy::randomIP() {
    // get prefix
    struct prefix p = getPrefix();
    qDebug() << "prefix " <<  p.str;

    // generate random IP
    static bool first = true;
    if (first) {
        srand(time(NULL));
        first = false;
    } else {
        srand(rand());
    }
    char* prefixBuff = p.str.toUtf8().data();
    char v6buf[40];
    strcpy(v6buf, prefixBuff);

    // we don't bother with bits but just use blocks of bytes after the "prefix"
    int nbLeft = (128 - p.len) / 4; // divide by 4 since each char represents 4 bits in hex
    //int nbLeft = 40 - strlen(prefixBuff);
    int index = p.str.length();

    int column = 0;
    while (nbLeft > 0) {
        if (column == 4) {
            v6buf[index] = ':';
            index++;
            column = 0;
            continue;
        }
        v6buf[index] = intToHexChar(rand() % 16);
        index++;
        column++;
        nbLeft--;
    }
    v6buf[index] = '\0';

    QString toRet(v6buf);
    toRet.truncate(toRet.length()); // XXX investigate why "-2"

    toRet = toRet % "/" + QString::number(p.len);

    qDebug() << "new random Ip" << toRet;
    return toRet;
}

Proxy* Proxy::getProxy(QString md5) {
    if (proxyHashes.contains(md5)) {
        return proxyHashes.value(md5);
    }
    return NULL;
}

void Proxy::sendRawFinish(int exitCode) {
    // TODO
    // TODO check correspondance of exit code (seems like 255 -3 = 252 ?)
    qWarning() << "sendRaw helper had an error " << exitCode;

    if (exitCode == 3) {
        //qDebug() << sendRaw.readAllStandardError();
    }
}

void Proxy::sendRawStandardError() {
    QProcess* sendRaw = dynamic_cast<QProcess*> (QObject::sender());
    qDebug() << "SEND RAW ERROR";
    qDebug() << sendRaw->readAllStandardError();
}

void Proxy::sendRawStandardOutput() {
    QProcess* sendRaw = dynamic_cast<QProcess*> (QObject::sender());
    qDebug() << "SEND RAW OUTPUT";
    qDebug() << sendRaw->readAllStandardOutput();
}

void Proxy::readyRead() {
    QProcess* pcap = dynamic_cast<QProcess*> (QObject::sender());
start: // used to process packets when bytes are available but no signal will be called
    qDebug() << "Got in readyread";
    char lenBuffer[20];
    if (left == 0) {
        int nbBufferedChars = 0;
        // get length
        char c;
        if (!pcap->getChar(&c)) {
            pcap->ungetChar(c);
            qDebug() << "ungetchar!";
            return; // wait for more!
        }
        if (c != '[') {
            qWarning() << "format error";
            return;
        }
        bool first = true;
        while (c != ']') {
            lenBuffer[nbBufferedChars] = c;
            nbBufferedChars++;
            if (!first && !isdigit(c)) {
                qWarning() << "format error, digit required between []";
                return;
            }
            first = false; // avoid the isdigit check for c == '['
            if (!pcap->getChar(&c)) {
                for (int i = nbBufferedChars - 1; i >= 0; i--) {
                    qDebug() << "ungetchar!";
                    pcap->ungetChar(lenBuffer[i]);
                }
                pcap->ungetChar(c);
                return; // wait for more!
            }
        }
        if (nbBufferedChars == 1) {
            qWarning() << "format error, empty packet length!";
            return;
        }
        lenBuffer[nbBufferedChars] = ']';
        lenBuffer[nbBufferedChars + 1] = '\0';
        qDebug() << "lenbuffer" << lenBuffer;
        char size[20];
        bzero(size, sizeof(size));
        for (int k = 1; k < nbBufferedChars; k++) {
            size[k - 1] = lenBuffer[k];
        }
        qDebug() << size;
        left = atoi(size);
    }

    qDebug() <<  pcap->bytesAvailable();
    if (pcap->bytesAvailable() < left) {
        qDebug() << "waiting for more bytes";
        return; // wait for more bytes
    }

    // get packet and send it to dtls connection
    char iface[5]; // we don't use pcap iface argument because we rely on the pcap datalink type, safer
    char c;
    int i = 0;
    while ((pcap->getChar(&c)) & (c !='\r')) { // fetch iface
        iface[i] = c;
        left--;
        i++;
    }
    pcap->getChar(&c); // remove the '\n'
    left -= 2; // the '\r\n'
    qDebug() << iface << "iface!";

    char srcIp[40];
    char mac[18];
    mac[0] = '\0'; // to detect if empty mac

    // fetch srcIp
    i = 0;
    while ((pcap->getChar(&c)) & (c !='\r')) {
        srcIp[i] = c;
        i++;
        left--;
    }
    pcap->getChar(&c); // remove the '\n'
    left -= 2; // the '\r\n'
    srcIp[i] = '\0'; // replace '\n' by '\0'

    if (QString(iface) == "eth") { // ethernet, fetch MAC addr
        i = 0;
        while ((pcap->getChar(&c)) & (c !='\r')) {
            mac[i] = c;
            i++;
            left--;
        }
        pcap->getChar(&c); // remove the '\n'
        left -= 2; // the '\r\n'
        mac[i] = '\0';

        qDebug() << "Resolver adds mapping " << mac << srcIp;
        // add to IpResolver
        resolver->addMapping(srcIp, mac, pcap->arguments().at(0));
    }

    char packet[2000];
    pcap->read(packet, left);

    if (port != listenPort) {
        // first 16 bits = source Port of UDP and TCP
        qint16* dstPort = static_cast<qint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
        *dstPort = htons(listenPort); // restore the original port
    }

    char packetAndLen[2020];
    sprintf(packetAndLen, "[%d]", left);
    int sizeOfLen = strlen(packetAndLen); // we need to know the [size] string length for the memcpy
    memcpy(packetAndLen + sizeOfLen + 1, packet, left);

    // send over DTLS with friendUid
    QFile tcpPacket("tcpPackeFromPcap");
    tcpPacket.open(QIODevice::WriteOnly);
    tcpPacket.write(packetAndLen, left + sizeOfLen + 1);

    qDebug() << "source IP" << srcIp;
    qDebug() << "source MAC" << mac;
    QString qsrcIp(srcIp);

    con->sendBytes(packetAndLen, left + sizeOfLen + 1, idHash, sockType, qsrcIp);

    // send over raw! (test)
    //this->sendBytes(packet, left);

    left = 0;
    if (pcap->bytesAvailable() > 0) {
        qDebug() << "There are" << pcap->bytesAvailable() << "bytes left";
        goto start;
    }
}

void Proxy::pcapFinish(int exitCode) {
    // TODO
    // TODO check correspondance of exit code (seems like 255 -3 = 252 ?)
    qWarning() << "pcap exited with exit code " << exitCode;
}

void Proxy::run_pcap() {
    port = listenPort;
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

    bool bound = false;
    while (!bound) {
        // create socket and bind for the kernel
        QProcess* bindSocket = new QProcess(this);
        QStringList bindSocketArgs;
        bindSocketArgs.append(QString::number(sockType));
        bindSocketArgs.append(QString::number(ipProto));
        bindSocketArgs.append(QString::number(port));
        bindSocketArgs.append(listenIp);
        qDebug() << "bind socket args" << bindSocketArgs;
        bindSocket->start(QString(HELPERPATH) + "newSocket", bindSocketArgs);
        if (bindSocket->waitForFinished(400)) { // 200ms should be plenty enough for the process to fail on bind
            if (bindSocket->exitCode() == EADDRNOTAVAIL) {
                // loop again until IP is available but just sleep a moment
                QThread::sleep(1);
            } else if (bindSocket->exitCode() == 3) {
                qDebug() << "exit code!" << bindSocket->exitCode();
                qDebug() << bindSocket->readAllStandardError();
                if (port < 60000) {
                    port = 60001;
                } else {
                    port++;
                }
                if (port >= 65535)
                    qFatal("Port escalation higher than 65535");
                qDebug() << "Could not bind on port " << listenPort << "gonna use " << QString::number(port);
                bindSocket->deleteLater();
            } else {
                qFatal("Error with the bind helper process");
                exit(-1);
            }
        } else {
            bound = true;
            u->addQProcess(bindSocket);
        }
    }
        // listen for packets with pcap, forward on the secure UDP link
        QStringList args;
    #ifdef __APPLE__
        args.append("lo0"); // TODO get default iface AND listen also on lo !
    #elif __GNUC__
        //args.append("lo"); // TODO get default iface AND listen also on lo !
        args.append("eth0"); // TODO get default iface AND listen also on lo !
    #endif
        QString transportStr;
        sockType == SOCK_DGRAM ? transportStr = "udp" : transportStr = "tcp";
        args.append("ip6 dst host " + listenIp + " and " + transportStr + " and dst port " + QString::number(port));
        QProcess* pcap = new QProcess(this);
        connect(pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));
        // TODO connect pcap to deleteLater ?
        connect(pcap, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
        qDebug() << args;

        u->addQProcess(pcap); // add pcap to be killed when main is killed

        pcap->start(QString(HELPERPATH) + "pcapListen", args);
}


