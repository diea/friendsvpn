#include <QApplication>
#include "graphic/systray.h"
#include "bonjourbrowser.h"
#include "bonjourdiscoverer.h"
#include "bonjoursql.h"
#include "poller.h"
#include "rawsocket.h"
#include "controlplane/connectioninitiator.h"
#include "controlplane/controlplaneserver.h"
#include "controlplane/controlplaneclient.h"
#include <QDebug>

#if 0
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // start systray
    SysTray* st = new SysTray();

    // connect to sql database
    BonjourSQL* qSql = new BonjourSQL();

    // create facebook app xmlrpc poller
    PollerController* polCtrl = new PollerController(qSql);
    emit polCtrl->operate();
    //poller->run();
    // listen for commands from facebook app
    //AppListener* app = new AppListener(qSql);

    // get uid from app
    qSql->uidOK();

    // discover services
    BonjourDiscoverer* disco = BonjourDiscoverer::getInstance(qSql, qApp);
    disco->discoverServices();

    // open dataplane tunnel


    return a.exec();
}
#elif 1
// test main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // start systray
    //SysTray* st = new SysTray();

    // connect to sql database
    BonjourSQL* qSql = new BonjourSQL();

    // create facebook app xmlrpc poller
    QThread pollerThread;
    Poller* poller = new Poller(qSql);
    poller->moveToThread(&pollerThread);
    poller->connect(&pollerThread, SIGNAL(started()), SLOT(run()));
    pollerThread.start();

    // get uid from app
    qSql->uidOK();

    // Control plane
    ConnectionInitiator* con = ConnectionInitiator::getInstance(qSql);
    con->run();

    return a.exec();
}
#endif
