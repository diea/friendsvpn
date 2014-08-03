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
#include <QPixMap>
#include <QSplashScreen>

/* used for test */
#include "proxyserver.h"
#include "helpers/raw_structs.h"

#ifndef QT_NO_DEBUG_OUTPUT
QTextStream out;

void logOutput(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
    static QMutex logMutx;
    logMutx.lock();
    QString debugdate = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss");
    switch (type) {
    case QtDebugMsg:
#ifdef NODEBUG
        logMutx.unlock();
        return;
#endif
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

    out << debugdate << " " << msg << endl;

    if (QtFatalMsg == type) {
        UnixSignalHandler::termSignalHandler(0);
    }
    logMutx.unlock();
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifndef QT_NO_DEBUG_OUTPUT /* used to log timestamps test */
    QString fileName = QCoreApplication::applicationDirPath() + "/../friendsvpn.log";
    QFile *log = new QFile(fileName);
    if (log->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        out.setDevice(log);
        qInstallMessageHandler(&logOutput);
    } else {
        qDebug() << "Error opening log file '" << fileName << "'. All debug output redirected to console.";
    }
#endif
    a.setQuitOnLastWindowClosed(false);
    // init signal handler
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

    // start systray
    QThread sysTrayThread;
    SysTray* st = SysTray::getInstance();
    st->run();

    qDebug() << "----------------------------------------- START APPLICATION -----------------------------------------";
    QPixmap pixmap(":images/splash.png");
    QSplashScreen *splash = new QSplashScreen(pixmap);
    splash->show();
    // connect to sql database
    DatabaseHandler* qSql = DatabaseHandler::getInstance();

    // create facebook app xmlrpc poller
    QThread pollerThread;
    Poller* poller = Poller::getInstance();
    poller->moveToThread(&pollerThread);
    poller->connect(&pollerThread, SIGNAL(started()), SLOT(run()));
    QObject::connect(u, SIGNAL(exiting()), &pollerThread, SLOT(quit()), Qt::DirectConnection);
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
    QObject::connect(u, SIGNAL(exiting()), &discovererThread, SLOT(quit()), Qt::DirectConnection);
    discovererThread.start();

    con->run();
    splash->close();
    return a.exec();
}
