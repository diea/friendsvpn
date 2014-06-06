#include <QApplication>
#include "graphic/systray.h"
#include "bonjour/bonjourdiscoverer.h"
#include "databasehandler.h"
#include "poller.h"
#include "connectioninitiator.h"
#include "unixsignalhandler.h"
#include "proxy.h"
#if QT_VERSION >= 0x50000 
#include <QtConcurrent>
#endif

/* used for test */
#include "proxyserver.h"
#ifndef QT_NO_DEBUG_OUTPUT

QTextStream *out = 0;

void logOutput(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
    printf("Logoutput\n");
    QString debugdate = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss");
    switch (type)
    {
    case QtDebugMsg:
        debugdate += "[D]";
        break;
    case QtWarningMsg:
        debugdate += "[W]";
        break;
    case QtCriticalMsg:
        debugdate += "[C]";
        break;
    case QtFatalMsg:
        debugdate += "[F]";
    }
    (*out) << debugdate << " " << msg << endl;

    if (QtFatalMsg == type)
    {
        abort();
    }
    printf("Finished logoutput\n");
    fflush(stdout);
    return;
}
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifndef QT_NO_DEBUG_OUTPUT /* used to log timestamps test */
    QString fileName = "friendsvpn.log";
    QFile *log = new QFile(fileName);
    if (log->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        out = new QTextStream(log);
        qInstallMessageHandler(&logOutput);
    } else {
        qDebug() << "Error opening log file '" << fileName << "'. All debug output redirected to console.";
    }
#endif

    a.setQuitOnLastWindowClosed(false);

    // init signal handler
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

#if 1

#ifdef __APPLE__ /* linux only in text mode */
    // start systray
    QThread sysTrayThread;
    SysTray* st = new SysTray();
    QObject::connect(&sysTrayThread, SIGNAL(started()), st, SLOT(createActions()));
    QObject::connect(&sysTrayThread, SIGNAL(started()), st, SLOT(createTrayIcon()));
    sysTrayThread.start();
    QObject::connect(u, SIGNAL(exiting()), &sysTrayThread, SLOT(quit()));
#endif

    // connect to sql database
    DatabaseHandler* qSql = DatabaseHandler::getInstance();

    // create facebook app xmlrpc poller
    QThread pollerThread;
    Poller* poller = new Poller();
    poller->moveToThread(&pollerThread);
    poller->connect(&pollerThread, SIGNAL(started()), SLOT(run()));
    QObject::connect(u, SIGNAL(exiting()), &pollerThread, SLOT(quit()));
    pollerThread.start();

    // get uid from app
    qSql->uidOK();

    Proxy::gennewIP(); // generate the initial new ips

    ConnectionInitiator* con = ConnectionInitiator::getInstance();

    // discover services
    QThread discovererThread;
    BonjourDiscoverer* disco = BonjourDiscoverer::getInstance();
    disco->moveToThread(&discovererThread);
    QObject::connect(&discovererThread, SIGNAL(started()), disco, SLOT(discoverServices()));
    QObject::connect(u, SIGNAL(exiting()), &discovererThread, SLOT(quit()));
    discovererThread.start();

    con->run();
#endif
    return a.exec();
}
