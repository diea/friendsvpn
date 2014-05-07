#include "proxyserver.h"
#include <QCryptographicHash>
#include "bonjour/bonjourrecord.h"
#include "unixsignalhandler.h"

QHash<QString, ProxyServer*> ProxyServer::proxyHashes;

ProxyServer::ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                       const QString &hostname, quint16 port) :
    Proxy()
{
    QString allParams = friendUid + name + regType + domain + hostname + QString::number(port);

    QByteArray hash = QCryptographicHash::hash(allParams.toUtf8().data(), QCryptographicHash::Md5);
    idHash = QString(hash);
    if (proxyHashes.contains(idHash)) {
        throw 1; // already exists, we throw int "1"
    }
    proxyHashes.insert(idHash, this);

    qDebug() << "Proxy constructor!";
    QString newip = Proxy::newIP();

    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newip);
    rec = BonjourRecord(name, regType, domain, hostname, ip, port);
    left = 0;

    // TODO
    sockType = SOCK_STREAM;
    ipProto = IPPROTO_TCP;

    // get DataPlaneConnection associated with friendUid
    ConnectionInitiator* initiator = ConnectionInitiator::getInstance();
    con = initiator->getDpConnection(friendUid);
}

void ProxyServer::run() {
    int port = rec.port;
        UnixSignalHandler* u = UnixSignalHandler::getInstance();

        bool bound = false;
        while (!bound) {
            // create socket and bind for the kernel
            QProcess* bindSocket = new QProcess(this);
            QStringList bindSocketArgs;
            bindSocketArgs.append(QString::number(sockType));
            bindSocketArgs.append(QString::number(ipProto));
            bindSocketArgs.append(QString::number(port));
            bindSocketArgs.append(rec.ips.at(0));
            qDebug() << "bind socket args" << bindSocketArgs;
            bindSocket->start(QString(HELPERPATH) + "newSocket", bindSocketArgs);
            if (bindSocket->waitForFinished(200)) { // 200ms should be plenty enough for the process to fail on bind
                if (bindSocket->exitCode() == 3) {
                    if (port < 60000) {
                        port = 60000 + port;
                    } else {
                        port++;
                    }
                    if (port >= 65535)
                        qFatal("Port escalation higher than 65535");
                    qDebug() << "Could not bind on port " << rec.port << "gonna use " << QString::number(port);
                    bindSocket->deleteLater();
                }
            } else {
                bound = true;
                u->addQProcess(bindSocket);
            }
        }
    #if 0 // this is now done inside the bindSocket process => need root to bind to local ports
        struct sockaddr_in6 serv_addr;
        int fd = socket(AF_INET6, sockType, ipProto);
        if (fd < 0) {
            qWarning() << "Could not open socket";
            return;
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        int portno = rec.port;
        char* listenAddr = rec.ips.at(0).toUtf8().data();
        inet_pton(AF_INET6, (const char *) &listenAddr, &(serv_addr.sin6_addr));
        serv_addr.sin6_family = AF_INET6;
        serv_addr.sin6_port = htons(portno);
        if (bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            qWarning() << "ERROR on binding";
            return;
        }
    #endif
        // listen for packets with pcap, forward on the secure UDP link
        QStringList args;
    #ifdef __APPLE__
        args.append("lo0"); // TODO get default iface AND listen also on lo !
    #elif __GNUC__
        args.append("lo"); // TODO get default iface AND listen also on lo !
    #endif
        QString transportStr;
        sockType == SOCK_DGRAM ? transportStr = "udp" : transportStr = "tcp";
        args.append("ip6 dst host " + rec.ips.at(0) + " and " + transportStr + " and dst port " + QString::number(port));
        QProcess* pcap = new QProcess(this);
        connect(pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));
        // TODO connect pcap to deleteLater ?
        connect(pcap, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
        qDebug() << args;

        u->addQProcess(pcap); // add pcap to be killed when main is killed

        pcap->start(QString(HELPERPATH) + "pcapListen", args);

        /*
        QStringList sendRawArgs;
        sendRawArgs.append(QString::number(sockType));
        sendRawArgs.append(QString::number(ipProto));
        sendRawArgs.append("fd3b:e180:cbaa:1:156e:72ed:97d0:a60");

        connect(&sendRaw, SIGNAL(finished(int)), this, SLOT(sendRawFinish(int)));
        sendRaw.start(QString(HELPERPATH) + "sendRaw", sendRawArgs);
        */
        // advertise by registering the record with a bonjour registrar
        // TODO
}

void ProxyServer::pcapFinish(int exitCode) {
    // TODO
    // TODO check correspondance of exit code (seems like 255 -3 = 252 ?)
    qWarning() << "pcap exited with exit code " << exitCode;
}

void ProxyServer::sendRawFinish(int exitCode) {
    // TODO
    // TODO check correspondance of exit code (seems like 255 -3 = 252 ?)
    qWarning() << "sendRaw helper had an error " << exitCode;

    if (exitCode == 3) {
        //qDebug() << sendRaw.readAllStandardError();
    }
}

void ProxyServer::readyRead() {
    QProcess* pcap = dynamic_cast<QProcess*> (QObject::sender());

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

    // send over DTLS with friendUid
    con->sendBytes(packet, left);

    left = 0;
}
