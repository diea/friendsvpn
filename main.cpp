#include <QApplication>
#include "graphic/systray.h"
#include "bonjourbrowser.h"
#include "bonjourdiscoverer.h"
#include "bonjoursql.h"
#include "poller.h"
#include "pollercontroller.h"
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
#else if 1
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
    PollerController* polCtrl = new PollerController(qSql);
    emit polCtrl->operate();
    //poller->run();
    // listen for commands from facebook app
    //AppListener* app = new AppListener(qSql);

    // get uid from app
    qSql->uidOK();

    // test ip raw socket
    //RawSocket raw(30000);

    // test SSL server
    //qDebug() << "Main Thread : " << QThread::currentThreadId();
    //ControlPlaneServer* con = new ControlPlaneServer(QHostAddress("192.168.1.64"), 61323);
    //con->start();
    ConnectionInitiator con(qSql, qApp);
    con.run();

    /*QThread clientCon;
    ControlPlaneClient* cli = new ControlPlaneClient(QHostAddress("192.168.1.64"), 61323);
    cli->moveToThread(&clientCon);
    cli->connect(&clientCon, SIGNAL(started()), SLOT(run()));
    clientCon.start();*/
    //sleep(5);
    //qDebug() << "Deleting!";
    //delete con;
    return a.exec();
}
#endif
