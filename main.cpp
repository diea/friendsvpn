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
#include <QDebug>
#if QT_VERSION >= 0x50000 
#include <QtConcurrent>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // init signal handler
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

#if 1
#ifdef __APPLE__
    // start systray
    QThread sysTrayThread;
    SysTray* st = new SysTray();
    QObject::connect(&sysTrayThread, SIGNAL(started()), st, SLOT(createActions()));
    QObject::connect(&sysTrayThread, SIGNAL(started()), st, SLOT(createTrayIcon()));
    sysTrayThread.start();
    QObject::connect(u, SIGNAL(exiting()), &sysTrayThread, SLOT(quit()));
#endif

    // connect to sql database
    BonjourSQL* qSql = BonjourSQL::getInstance();

    // create facebook app xmlrpc poller
    QThread pollerThread;
    Poller* poller = new Poller();
    poller->moveToThread(&pollerThread);
    poller->connect(&pollerThread, SIGNAL(started()), SLOT(run()));
    QObject::connect(u, SIGNAL(exiting()), &pollerThread, SLOT(quit()));
    pollerThread.start();


    // get uid from app
    qSql->uidOK();

    QtConcurrent::run(Proxy::gennewIP); // generate the initial new ips

    ConnectionInitiator* con = ConnectionInitiator::getInstance();

    // discover services
    QThread discovererThread;
    BonjourDiscoverer* disco = BonjourDiscoverer::getInstance();
    disco->moveToThread(&discovererThread);
    QObject::connect(&discovererThread, SIGNAL(started()), disco, SLOT(discoverServices()));
    QObject::connect(u, SIGNAL(exiting()), &discovererThread, SLOT(quit()));
    discovererThread.start();
    disco->discoverServices();

    con->run();
#endif
    return a.exec();
}
