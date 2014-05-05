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
#include <QDebug>

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

    BonjourSQL* qSql = BonjourSQL::getInstance();

    ConnectionInitiator* con = ConnectionInitiator::getInstance();
    con->run();



    return a.exec();
}
#endif
