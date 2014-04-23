#include <QApplication>
#include "graphic/systray.h"
#include "bonjour/bonjourbrowser.h"
#include "bonjour/bonjourdiscoverer.h"
#include "bonjoursql.h"
#include "poller.h"
#include "rawsocket.h"
#include "controlplane/connectioninitiator.h"
#include "controlplane/controlplaneserver.h"
#include "controlplane/controlplaneclient.h"
#include "bonjour/bonjourregistrar.h"
#include "dataplane/dataplaneconnection.h"
#include "dataplane/dataplaneclient.h"
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
    /*qDebug() << "new Registrar";
    BonjourRegistrar test;

    QList<QString> ip;
    ip.append("fd3b:e180:cbaa:1:5e96:9dff:fe8a:8448");
    BonjourRecord newRec("monpremiertest", "_http._tcp", "local.", "spark",
                         ip, 80);
    test.registerService(newRec);
    qDebug() << "registered!";*/

    BonjourSQL* qSql = BonjourSQL::getInstance();
    // get uid from app
    qSql->uidOK();

    DataPlaneConnection* dp = DataPlaneConnection::getInstance(qApp);
    dp->start();
    /*QList<QString> ip;
    ip.append("fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447");
    BonjourRecord newRec("monpremiertest", "_http._tcp", "local.", "spark",
                         ip, 80);
    DataPlaneClient* dc = new DataPlaneClient(newRec, qApp);
    dc->sendBytes("TEST", 4);
    return a.exec();*/
}
#endif
