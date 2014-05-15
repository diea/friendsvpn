#include <QApplication>
#include "graphic/systray.h"
#include "bonjour/bonjourbrowser.h"
#include "bonjour/bonjourdiscoverer.h"
#include "bonjoursql.h"
#include "poller.h"
#include "connectioninitiator.h"
#include "controlplane/controlplaneserver.h"
#include "controlplane/controlplaneclient.h"
#include "bonjour/bonjourregistrar.h"
#include "dataplane/dataplaneserver.h"
#include "dataplane/dataplaneclient.h"
#include "proxy.h"
#include "proxyserver.h"
#include "unixsignalhandler.h"
#include "ipresolver.h"
//#include "testextend.h"
#include <QDebug>
#include <QtConcurrent>

#if 0
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // start systray
    //SysTray* st = new SysTray();

    // connect to sql database
    BonjourSQL* qSql = BonjourSQL::getInstance();

    // create facebook app xmlrpc poller
    QThread pollerThread;
    Poller* poller = new Poller();
    poller->moveToThread(&pollerThread);
    poller->connect(&pollerThread, SIGNAL(started()), SLOT(run()));
    pollerThread.start();

    // get uid from app
    qSql->uidOK();

    // discover services
    BonjourDiscoverer* disco = BonjourDiscoverer::getInstance(qApp);
    disco->discoverServices();

    // Control plane
    ConnectionInitiator* con = ConnectionInitiator::getInstance();
    con->run();

    return a.exec();
}
#elif 1


// test main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // init signal handler
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

    /*IpResolver* ip = IpResolver::getInstance();
    struct ip_mac_mapping map = ip->getMapping("192.168.1.127");
    qDebug() << map.interface;*/

    BonjourSQL* qSql = BonjourSQL::getInstance();

    QtConcurrent::run(Proxy::gennewIP); // generate the initial new ips

    // discover services
    QThread discovererThread;
    BonjourDiscoverer* disco = BonjourDiscoverer::getInstance();
    disco->moveToThread(&discovererThread);
    QObject::connect(&discovererThread, SIGNAL(started()), disco, SLOT(discoverServices()));
    discovererThread.start();
    //disco->discoverServices();

    ConnectionInitiator* con = ConnectionInitiator::getInstance();
    con->run();

   /* ProxyServer* newProxy1 = new ProxyServer("100008078109463", "diea-VirtualBox-3", "_http._tcp.", ".friendsvpn.", "diea-VirtualBox-3.local", 45940);
    newProxy1->run();*/
    // no move to thread ?

    /*QThread* newProxyThread = new QThread();
    ProxyServer* newProxy = new ProxyServer("100008078109463", "diea-VirtualBox-3", "_http._tcp.", ".friendsvpn.", "diea-VirtualBox-3.local", 80);
    // no move to thread ?
    QObject::connect(newProxyThread, SIGNAL(started()), newProxy, SLOT(run()));
    QObject::connect(newProxyThread, SIGNAL(finished()), newProxyThread, SLOT(deleteLater()));
    newProxyThread->start();*/

    /*QFile serv("serverRcvdPacket71");
    serv.open(QIODevice::ReadOnly);
    QByteArray bytes = serv.readAll();

    struct rawComHeader* head = static_cast<struct rawComHeader*>(static_cast<void*>(bytes.data()));
    qDebug() << head->len;*/

   /* QFile httpData("httpdata");
    httpData.open(QIODevice::ReadOnly);
    QByteArray bytes = httpData.readAll();
    char* buf = bytes.data();
    char newBuf[500];
    uint32_t size;
    memset(&size, 0, sizeof(uint32_t));
    size = 324;
    memcpy(newBuf, &size, sizeof(uint32_t));
    memcpy(newBuf + sizeof(uint32_t), buf, 324);
    httpData.close();
    httpData.open(QIODevice::WriteOnly);
    httpData.write(newBuf, 324 + sizeof(uint32_t));
    httpData.close();*/

#if 0
    QProcess sendRaw;
    QStringList sendRawArgs;

    sendRawArgs.append("lo0");
    sendRawArgs.append("fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447");
    sendRawArgs.append("::1");
    sendRawArgs.append(QString::number(1));
    sendRawArgs.append(QString::number(51423));

    sendRaw.start(QString(HELPERPATH) + "sendRaw", sendRawArgs);

    sendRaw.waitForStarted();

    while (1) {
        QFile serv("testRawHead");
        serv.open(QIODevice::ReadOnly);
        QByteArray bytes = serv.readAll();

        qDebug() << "write!" << sendRaw.state();
        qDebug() << "writing " << bytes.length();
        char* buf = bytes.data();

        struct rawComHeader* head = static_cast<struct rawComHeader*>(static_cast<void*>(buf));
        qDebug() << "Header length" << head->len;

        sendRaw.write(buf, bytes.length());
        sendRaw.waitForBytesWritten();
        //sendRaw.waitForReadyRead();
        //qDebug() << sendRaw.readAllStandardOutput();
       // QThread::sleep(5);
    }
#endif
    return a.exec();
}
#endif
