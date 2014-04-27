#include "proxy.h"

Proxy::Proxy(const QString &name, const QString &regType, const QString &domain,
             const QString &hostname, quint16 port, QObject *parent) :
    QObject(parent)
{
    // generate random ULA
    QString newIp = randomULA();
    // add it with ifconfig
    QProcess ifconfig;
    // TODO include in the app bundle to launch from there
    ifconfig.start("/Users/diea/Dropbox/CiscoVPN/app/friendsvpn/helper/ifconfighelp");
    ifconfig.waitForReadyRead();
    qDebug() << ifconfig.readAllStandardOutput();
    qDebug() << ifconfig.readAllStandardError();
    ifconfig.close();

    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newIp);
    rec = BonjourRecord(name, regType, domain, hostname, ip, port);
    left = 0;

    // TODO
    sockType = SOCK_STREAM;
    ipProto = IPPROTO_TCP;

}

void Proxy::run() {
    // create socket and bind for the kernel
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
    // listen for packets with pcap, forward on the secure UDP link
    QStringList args;
    args.append("lo0"); // TODO get default iface
    QString transportStr;
    sockType == SOCK_DGRAM ? transportStr = "udp" : transportStr = "tcp";
    args.append("ip6 host " + rec.ips.at(0) + " and " + transportStr + " and port " + QString::number(rec.port));

    connect(&pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));
    connect(&pcap, SIGNAL(readyReadStandardOutput()), this, SLOT(readyRead()));
    qDebug() << args;
    pcap.start("/Users/diea/Dropbox/CiscoVPN/app/friendsvpn/helper/pcapListen", args);

    QStringList sendRawArgs;
    sendRawArgs.append(QString::number(sockType));
    sendRawArgs.append(QString::number(ipProto));
    sendRawArgs.append("fd3b:e180:cbaa:1:156e:72ed:97d0:a60");

    connect(&sendRaw, SIGNAL(finished(int)), this, SLOT(sendRawFinish(int)));
    sendRaw.start("/Users/diea/Dropbox/CiscoVPN/app/friendsvpn/helper/sendRaw", sendRawArgs);
    // advertise by registering the record with a bonjour registrar

}

QString Proxy::randomULA() {
    //BonjourSQL* qSql = BonjourSQL::getInstance();

    //qsrand(qSql->getLocalUid()); // use UID for seed
    // do this when rest is working.
    return "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8448";
}

void Proxy::pcapFinish(int exitCode) {
    // TODO
    // TODO check correspondance of exit code (seems like 255 -3 = 252 ?)
    qWarning() << "pcap helper had an error " << exitCode;
}

void Proxy::sendRawFinish(int exitCode) {
    // TODO
    // TODO check correspondance of exit code (seems like 255 -3 = 252 ?)
    qWarning() << "sendRaw helper had an error " << exitCode;

    if (exitCode == 3) {
        qDebug() << sendRaw.readAllStandardError();
    }
}

void Proxy::readyRead() {
    if (left == 0) {
        // get length
        char c;
        pcap.getChar(&c);
        if (c != '[') {
            qWarning() << "format error";
            return;
        }
        QString nb;

        pcap.getChar(&c);
        while (c != ']') {
            if (!isdigit(c)) {
                qWarning() << "format error, digit required between []";
                return;
            }
            nb.append(c);
            pcap.getChar(&c);
        }

        if (nb.isEmpty()) { qWarning() << "format error, empty packet length!"; return; }

        left = nb.toInt();
    }

    if (pcap.bytesAvailable() < left) {
        qDebug() << "waiting for more bytes";
        return; // wait for more bytes
    }
    // get packet and send it to dtls connection
    char packet[2000];
    pcap.read(packet, left);

    QString size("[");
    qDebug() << "size" << left;
    size.append(QString::number(left));
    size.append("]");
    sendRaw.write(size.toUtf8().data());
    sendRaw.write(packet, left);

    left = 0;
}
