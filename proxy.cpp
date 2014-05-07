#include "proxy.h"
#include "unixsignalhandler.h"
#include <QCryptographicHash>

QHash<QString, Proxy*> Proxy::proxyHashes;

QString Proxy::defaultIface = Proxy::getDefaultInterface();

Proxy::Proxy(int srcPort, int sockType)
    : listenPort(srcPort), sockType(sockType)
{
    listenIp = newIP();

    if (sockType == SOCK_STREAM) ipProto = IPPROTO_TCP;
    else ipProto = IPPROTO_UDP;
    initRaw();
}

Proxy::Proxy(int srcPort, const QString& regType) : listenPort(srcPort)
{
    if (regType.contains("tcp")) {
        sockType = SOCK_STREAM;
        ipProto = IPPROTO_TCP;
    } else {
        sockType = SOCK_DGRAM;
        ipProto = IPPROTO_UDP;
    }
    initRaw();
}

void Proxy::initRaw() {
    // init raw socket to send packets
    sendRaw = new QProcess(this);
    QStringList sendRawArgs;
    sendRawArgs.append(QString::number(sockType));
    sendRawArgs.append(QString::number(ipProto));
    sendRawArgs.append(listenIp);

    connect(sendRaw, SIGNAL(finished(int)), this, SLOT(sendRawFinish(int)));

    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    u->addQProcess(sendRaw);

    sendRaw->start(QString(HELPERPATH) + "sendRaw", sendRawArgs);
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
                if (value == newip) {
                    newIp = false;
                    qDebug() << "local duplicate!";
                }
            }
        }
        testIfconfig.close();
    } while (!newIp);

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
    srand(time(NULL));
    char* prefixBuff = p.str.toUtf8().data();
    char v6buf[40];
    strcpy(v6buf, prefixBuff);

    // we don't bother with bits but just use blocks of bytes after the "prefix"
    int nbLeft = (128 - p.len) / 4; // divide by 4 since each char represents 4 bits in hex
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
    v6buf[39] = '\0';

    QString toRet(v6buf);
    toRet.truncate(toRet.length() - 2); // XXX investigate why "-2"
    return toRet;
}

void Proxy::sendBytes(const char *buf, int len) {
    // use raw sock to send the bytes
    QString size("[");
    size.append(QString::number(len));
    size.append("]");
    sendRaw->write(size.toUtf8().data());
    sendRaw->write(buf, len);
}

Proxy* Proxy::getProxy(QString md5) {
    if (proxyHashes.contains(md5)) {
        return proxyHashes.value(md5);
    }
    return NULL;
}
