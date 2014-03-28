#include <QApplication>
#include "graphic/systray.h"
#include "bonjourbrowser.h"
#include "bonjourdiscoverer.h"
#include "bonjoursql.h"
#include "poller.h"
#include "pollercontroller.h"
#include "rawsocket.h"

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

    return a.exec();
}
#else if 1
// test main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // start systray
    SysTray* st = new SysTray();

    // test ip raw socket
    //RawSocket raw(30000);
    return a.exec();
}
#endif
