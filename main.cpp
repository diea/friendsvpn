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

/* define "getch" for use without ncurses */
/* code from http://stackoverflow.com/questions/7469139/what-is-equivalent-to-getch-getche-in-linux */
#include <termios.h>
#include <stdio.h>

static struct termios old, neww;

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  neww = old; /* make new settings same as old settings */
  neww.c_lflag &= ~ICANON; /* disable buffered i/o */
  neww.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &neww); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo)
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void)
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void)
{
  return getch_(1);
}
/* ---------------- end of getch ------------- */

/* used for test */
#include "proxyserver.h"
#ifndef QT_NO_DEBUG_OUTPUT

QTextStream out;

void logOutput(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
    static QMutex logMutx;
    logMutx.lock();
    //printf("Logoutput\n");
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

    out << debugdate << " " << msg << endl;

    if (QtFatalMsg == type)
    {
        abort();
    }

    logMutx.unlock();
    //printf("Finished logoutput\n");
    //fflush(stdout);
}
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#if 1
#ifndef QT_NO_DEBUG_OUTPUT /* used to log timestamps test */
    QString fileName = "friendsvpn.log";
    QFile *log = new QFile(fileName);
    if (log->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        //out = new QTextStream(log);
        out.setDevice(log);
        qInstallMessageHandler(&logOutput);
    } else {
        qDebug() << "Error opening log file '" << fileName << "'. All debug output redirected to console.";
    }
#endif
#endif
    a.setQuitOnLastWindowClosed(false);

    // init signal handler
    UnixSignalHandler* u = UnixSignalHandler::getInstance();

#if 1
#ifdef __APPLE__ /* linux only in text mode */
    // start systray
    QThread sysTrayThread;
    SysTray* st = SysTray::getInstance();
    QObject::connect(&sysTrayThread, SIGNAL(started()), st, SLOT(createActions()));
    QObject::connect(&sysTrayThread, SIGNAL(started()), st, SLOT(createTrayIcon()));
    sysTrayThread.start();
    QObject::connect(u, SIGNAL(exiting()), &sysTrayThread, SLOT(quit()), Qt::DirectConnection);
#endif

    // connect to sql database
    DatabaseHandler* qSql = DatabaseHandler::getInstance();

    // create facebook app xmlrpc poller
    QThread pollerThread;
    Poller* poller = new Poller();
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
#endif

    QChar chr(getch());
    if(chr == 3) {
        UnixSignalHandler::termSignalHandler(0);
    }

    return a.exec();
}
