#include "proxyserver.h"
#include <QCryptographicHash>
#include "bonjour/bonjourrecord.h"
#include "unixsignalhandler.h"
#include <arpa/inet.h>

QString ProxyServer::computeMd5(const QString &friendUid, const QString &name, const QString &regType,
                                 const QString &domain, const QString &hostname, quint16 port) {
    QString allParams = friendUid + name + regType + hostname + QString::number(port);

    QByteArray hash = QCryptographicHash::hash(allParams.toUtf8().data(), QCryptographicHash::Md5);
    //return QString(hash);
    return allParams; // TODO change
}

ProxyServer::ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                       const QString &hostname, quint16 port) : Proxy(port, regType,
                                                                      computeMd5(friendUid, name, regType, domain, hostname, port))
{
    qDebug() << "Proxy constructor!";

    QString newip = listenIp;
    // create bonjour rec with new IP
    QList<QString> ip;
    ip.append(newip);
    rec = BonjourRecord(name, regType, domain, hostname, ip, port);

    // get DataPlaneConnection associated with friendUid
    ConnectionInitiator* initiator = ConnectionInitiator::getInstance();
    con = initiator->getDpConnection(friendUid);
    connect(this, SIGNAL(newRaw(QString,int)), this, SLOT(serverNewRaw(QString, int)), Qt::QueuedConnection);
    qDebug() << "Proxy constructor threadId" << QThread::currentThreadId();
}

void ProxyServer::run() {
    qDebug() << "Proxy run threadId" << QThread::currentThreadId();
    run_pcap();

    // advertise by registering the record with a bonjour registrar
    // TODO
}

void ProxyServer::serverNewRaw(QString dstIp, int dstPort) {
    qDebug() << "Server New Raw threadId" << QThread::currentThreadId();
    clients.insert(dstIp, this->initRaw(dstIp, port));
}

void ProxyServer::sendBytes(const char *buf, int len, QString dstIp) {
    qDebug() << "Sending Bytes threadId" << QThread::currentThreadId();
    qDebug() << "I am a server sending bytes!";
    // is this a new client ?
    if (!clients.contains(dstIp)) {
        qDebug() << "new client !!";
        // make a new raw sock for dstIp
        emit serverNewRaw(dstIp, port); // do in original thread
        //clients.insert(dstIp, this->initRaw(dstIp, port));
    }
    mutex.lock();
    QProcess* sendRaw = clients.value(dstIp);
    while (!sendRaw) {
        sendRaw = clients.value(dstIp);
        mutex.unlock();
        QThread::msleep(200);
        mutex.lock();
    }
    qDebug() << "server writing buf" << buf;

    qDebug() << "sockRaw state " << sendRaw->state();

    QFile serv("serverRcvdPacket");
    serv.open(QIODevice::WriteOnly);
    serv.write(buf, len);
    // the srcPort is changed in the helper :)
    sendRaw->write(buf, len);
    mutex.unlock();
    /*
    // exit (TODO remove)
    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    u->termSignalHandler(3);
    */
}

void ProxyServer::receiveBytes(const char* buf, int len, int sockType, QString& srcIp) {
    con->sendBytes(buf, len, idHash, sockType, srcIp);
    qDebug() << "LEN" << len;
}
