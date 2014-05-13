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

    /*QThread* newProxyThread1 = new QThread();
    ProxyServer* newProxy1 = new ProxyServer("100008078109463", "diea-VirtualBox-3", "_workstation._tcp.", ".friendsvpn.", "diea-VirtualBox-3.local", 6000);
    // no move to thread ?
    QObject::connect(newProxyThread1, SIGNAL(started()), newProxy1, SLOT(run()));
    QObject::connect(newProxyThread1, SIGNAL(finished()), newProxyThread1, SLOT(deleteLater()));
    newProxyThread1->start();*/

    /*QThread* newProxyThread = new QThread();
    ProxyServer* newProxy = new ProxyServer("100008078109463", "diea-VirtualBox-3", "_http._tcp.", ".friendsvpn.", "diea-VirtualBox-3.local", 80);
    // no move to thread ?
    QObject::connect(newProxyThread, SIGNAL(started()), newProxy, SLOT(run()));
    QObject::connect(newProxyThread, SIGNAL(finished()), newProxyThread, SLOT(deleteLater()));
    newProxyThread->start();*/

    /*QFile test("tcpPackeFromPcap");
    test.open(QIODevice::ReadOnly);
    char* packetBuf = test.readAll().data();
    int accIndex = 4;

    quint16* srcPort = static_cast<quint16*>(malloc(sizeof(quint16)));
    memcpy(srcPort, packetBuf + accIndex, sizeof(quint16));
    *srcPort = ntohs(*srcPort);

    qDebug() << *srcPort;*/

    return a.exec();
}
#endif
