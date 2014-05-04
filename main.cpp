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
#if 0
    QThread* dcThread = new QThread(); // dataplane is threaded

    DataPlaneClient* dc = new DataPlaneClient(QHostAddress("::1"));
    QObject::connect(dcThread, SIGNAL(started()), dc, SLOT(run()));
    //QObject::connect(dcThread, SIGNAL(finished()), dcThread, SLOT(deleteLater()));
    dc->moveToThread(dcThread);
    dcThread->start();

    QThread* dcThread1 = new QThread(); // dataplane is threaded
    DataPlaneClient* dc1 = new DataPlaneClient(QHostAddress("fd3b:e180:cbaa:2:7058:3388:c3aa:8355"));
    QObject::connect(dcThread1, SIGNAL(started()), dc1, SLOT(run()));
    //QObject::connect(dcThread1, SIGNAL(finished()), dcThread1, SLOT(deleteLater()));
    dc1->moveToThread(dcThread1);
    dcThread1->start();

    /**/
#endif
    sleep(5);
    DataPlaneConnection* dp = con->getDpConnection("100008078109463");
    dp->sendBytes("Fack!\n", 6);
    return a.exec();
}
#endif
