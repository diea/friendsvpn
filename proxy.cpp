#include "proxy.h"
#include "unixsignalhandler.h"
#include "config.h"
#include <QCryptographicHash>
#include <string.h>
#include <QtConcurrent>
#include <pcap.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

QHash<QByteArray, Proxy*> Proxy::proxyHashes;
IpResolver* Proxy::resolver = IpResolver::getInstance();
RawSockets* Proxy::rawSockets = RawSockets::getInstance();
QString Proxy::defaultIface = IpResolver::getDefaultInterface();
QQueue<QString> Proxy::poolOfIps;
QMutex Proxy::poolOfIpsMutex;

Proxy::~Proxy() {
    /*while (!processes.empty()) {
        QProcess* p = processes.pop();
        p->terminate();
        p->waitForFinished(200);
        waitpid(p->pid(), NULL, WNOHANG);
        if (p->state() != QProcess::NotRunning) {
            p->kill();
        }
        UnixSignalHandler* u = UnixSignalHandler::getInstance();
        u->removeQProcess(p);
        if (p) {
            p->deleteLater();
        }
        p = NULL;
        proxyHashes.remove(idHash);
    }*/ // TODO
}

Proxy::Proxy(int srcPort, int sockType, QByteArray md5)
    : listenPort(srcPort), sockType(sockType)
{
    if (proxyHashes.contains(md5)) {
        throw 1; // already exists, we throw int "1"
    }
    idHash = md5;
    proxyHashes.insert(md5, this);

    listenIp = newIP();

    if (sockType == SOCK_STREAM) ipProto = IPPROTO_TCP;
    else ipProto = IPPROTO_UDP;

    pos = 0;
    remaining = 0;
}

Proxy::Proxy(int srcPort, const QString& regType, QByteArray md5) : listenPort(srcPort)
{
    if (proxyHashes.contains(md5)) {
        throw 1; // already exists, we throw int "1"
    }
    idHash = md5;
    proxyHashes.insert(md5, this);
    listenIp = newIP();
    if (regType.contains("tcp")) {
        sockType = SOCK_STREAM;
        ipProto = IPPROTO_TCP;
    } else {
        sockType = SOCK_DGRAM;
        ipProto = IPPROTO_UDP;
    }
}

QString Proxy::newIP() {
    poolOfIpsMutex.lock();
    while (poolOfIps.length() < 1) {
        qWarning() << "No ipv6 available!"; // should rarely happen!
        // it causes long delay for initiating connection if this happens
        poolOfIpsMutex.unlock();
        QtConcurrent::run(gennewIP);
        QThread::sleep(1);
        poolOfIpsMutex.lock();
    }
    QString newIp = poolOfIps.dequeue();
    int length = poolOfIps.length();
    poolOfIpsMutex.unlock();
    if (length < IP_BUFFER_LENGTH / 2) {
        QtConcurrent::run(gennewIP);
    }
    return newIp;
}

void Proxy::gennewIP() {
    poolOfIpsMutex.lock();
    int length = poolOfIps.length();
    poolOfIpsMutex.unlock();

    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    while (length < IP_BUFFER_LENGTH) {
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
                    }
                }
            }
            testIfconfig.close();
        } while (!newIp);

        // add it with ifconfig
        QProcess* ifconfig = new QProcess();
        u->addQProcess(ifconfig);
        QStringList newipArgs;
        newipArgs.append(defaultIface);
        newipArgs.append(newip);
        ifconfig->start(QString(HELPERPATH) + "ifconfighelp", newipArgs);
        ifconfig->waitForStarted();

        // wait 6 seconds for ifconfig to fail
        qDebug() << "Trying to create IP...";
        /*if (ifconfig->waitForFinished(6000)) {
            qDebug() << "new Duplicate! we generate an other one";
            u->removeQProcess(ifconfig);
            delete ifconfig;
        } else { TODO Uncomment*/
            qDebug() << "New IP created!";
            newip.truncate(newip.length() - 3); // remove prefix

            // add to local cache!
        #ifdef __APPLE__ // XXX better way of determining local loopback interface ?
            resolver->addMapping(newip, "", "lo0");
        #elif __GNUC__
            resolver->addMapping(newip, "", "lo");
        #endif

            poolOfIpsMutex.lock();
            poolOfIps.enqueue(newip);
            poolOfIpsMutex.unlock();
            length++;
        //}
    }
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
#ifdef TEST
            if (!list.at(1).startsWith("fe80") && list.at(1).startsWith("fd3b")) { // remove fd3b & maybe remove ULA.
#else
            if (!list.at(1).startsWith("fe80")) { // remove fd3b & maybe remove ULA.
#endif
                QString ip = list.at(1);
                ifconfig.close();

                struct prefix p;
                p.str = stripIp(ip, list.at(3).toInt());
                p.len = list.at(3).toInt();
                return p;
            }
#elif __GNUC__
#ifdef TEST
            if (!list.at(2).startsWith("fe80") && list.at(2).startsWith("fd3b")) {
#else
            if (!list.at(2).startsWith("fe80")) {
#endif
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
    toRet.truncate(toRet.length());

    toRet = toRet % "/" + QString::number(p.len);

    return toRet;
}

Proxy* Proxy::getProxy(QByteArray md5) {
    if (proxyHashes.contains(md5)) {
        return proxyHashes.value(md5);
    }
    return NULL;
}

void Proxy::sendRawFinish(int exitCode) {
    qWarning() << "sendRaw helper had an error " << exitCode;
}

void Proxy::sendRawStandardError() {
    QProcess* sendRaw = dynamic_cast<QProcess*> (QObject::sender());
    qDebug() << "SendRaw ERROR:";
    qDebug() << sendRaw->readAllStandardError();
}

void Proxy::sendRawStandardOutput() {
}

void Proxy::readyRead() {
    qFatal("got in proxy ready read, should not happen");
#if 0
    readyReadMut.lock();
    QProcess* pcap = dynamic_cast<QProcess*> (QObject::sender());
    //disconnect(pcap, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
    qDebug() << "Before reading header PCAP has" << pcap->bytesAvailable() << "bytes available";
    if (remaining <= 0) {
        if (pcap->bytesAvailable() < qint64(sizeof(pcapComHeader))) {
            qDebug() << "PCAP header was not available, not enough bytes to be read";
            readyReadMut.unlock();
            return; // wait for more!
        }
        pcap->read(static_cast<char*>(static_cast<void*>(&pcapHeader)), sizeof(struct pcapComHeader));
        //char packet[pcapHeader.len];
        packet = static_cast<char*>(malloc(pcapHeader.len * sizeof(char)));
        remaining = pcapHeader.len;
        pos = 0;
    }
    qint64 bytesAv = bytesAv = pcap->bytesAvailable();

    qDebug() << "PCAP has" << pcap->bytesAvailable() << "bytes available";
    qDebug() << "PCAP Header demands" << pcapHeader.len << "bytes";
    /* should not happen since we write everything in one fwrite in buffer */
    if (bytesAv >= remaining) {
        pcap->read(packet + pos, remaining);
        remaining = 0;
    } else {
        pos += bytesAv;
        remaining -= bytesAv;
        pcap->read(packet + pos, bytesAv);
        readyReadMut.unlock();
        return;
    }

    //pcap->read(packet, pcapHeader.len);

    if (port != listenPort) {
        // first 16 bits = source Port of UDP and TCP
        quint16* dstPort = static_cast<quint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
        *dstPort = htons(listenPort); // restore the original port
    }

    QString ipSrc(pcapHeader.ipSrcStr);

    qDebug() << "Receiving" << pcapHeader.len << "bytes from PCAP!";
    this->receiveBytes(packet, pcapHeader.len, sockType, ipSrc);

    readyReadMut.unlock(); /* reading finished, can begin next packet */

    //connect(pcap, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
    //readyReadMut.unlock(); /* TODO remove, used for debug */
#endif
}

void Proxy::pcapFinish(int exitCode) {
    qWarning() << "pcap exited with exit code " << exitCode;
}

void Proxy::run_pcap(const char* dstIp) {
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
        bindSocket->start(QString(HELPERPATH) + "newSocket", bindSocketArgs);

        if (bindSocket->waitForFinished(400)) { // 200ms should be plenty enough for the process to fail on bind
            if (bindSocket->exitCode() == EADDRNOTAVAIL) {
                // loop again until IP is available but just sleep a moment
                QThread::sleep(1);
            } else if (bindSocket->exitCode() == 3) {
                if (port < 60000) {
                    port = 60001;
                } else {
                    port++;
                }
                if (port >= 65535)
                    qFatal("Port escalation higher than 65535");
                qDebug() << "Could not bind on port " << listenPort << "going to use " << QString::number(port);
                bindSocket->deleteLater();
            } else {
                qFatal("Error with the bind helper process");
                exit(-1);
            }
        } else {
            bound = true;
            processes.push(bindSocket);
            u->addQProcess(bindSocket);
        }
    }

    qDebug() << "going to seek listen iface";
    QStack<QString> listenInterfaces;
    if (dstIp != 0) {
        struct ip_mac_mapping map = resolver->getMapping(dstIp);
        if (map.interface == "") {
            qDebug() << "No mapping for" << dstIp;
            return;
        }
        listenInterfaces.push(map.interface);
    } else {
        listenInterfaces = resolver->getActiveInterfaces();
    }


    while (!listenInterfaces.empty()) {
        // listen for packets with pcap, forward on the secure UDP link
        QStringList args;
        QString ifa = listenInterfaces.pop();
        args.append(ifa);
        QString transportStr;
        sockType == SOCK_DGRAM ? transportStr = "udp" : transportStr = "tcp";
        args.append("ip6 dst host " + listenIp + " and " + transportStr + " and dst port " + QString::number(port));

        QThread* pcapWorkerThread = new QThread();
        PcapWorker* pcapWorker = new PcapWorker(args, this);
        pcapWorker->moveToThread(pcapWorkerThread);
        connect(pcapWorkerThread, SIGNAL(started()), pcapWorker, SLOT(run()));
        connect(pcapWorkerThread, SIGNAL(finished()), pcapWorkerThread, SLOT(deleteLater()));
        pcapWorkers.push(pcapWorker);
        pcapWorkerThread->start();
    }
}


