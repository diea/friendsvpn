#include "unixsignalhandler.h"
#include "ipresolver.h"
#include "config.h"
#include "proxy.h"
#include <signal.h>
#include <QDebug>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <QThread>
#include <unistd.h>

UnixSignalHandler* UnixSignalHandler::instance = NULL;

UnixSignalHandler::UnixSignalHandler(QObject *parent) :
    QObject(parent)
{
    connect(this, SIGNAL(exiting()), this, SLOT(doExit()));
    setup_unix_signal_handlers();
}

UnixSignalHandler* UnixSignalHandler::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (instance == NULL) {
        instance = new UnixSignalHandler();
    }
    mutex.unlock();
    return instance;
}


int UnixSignalHandler::setup_unix_signal_handlers() {
    struct sigaction term;

    term.sa_handler = UnixSignalHandler::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &term, 0) > 0)
        return 1;
    if (sigaction(SIGTERM, &term, 0) > 0)
        return 2;

    return 0;
}

void UnixSignalHandler::addQProcess(QProcess *p) {
    listOfProcessToKill.append(p);
}

void UnixSignalHandler::removeQProcess(QProcess *p) {
    listOfProcessToKill.removeAll(p);
}

void UnixSignalHandler::addIp(QString ip) {
    listOfIps.append(ip);
}

void UnixSignalHandler::removeIp(QString ip) {
    QProcess cleanup;
    QStringList cleanArgs;
    cleanArgs.append(IpResolver::getDefaultInterface());
    cleanArgs.append(ip);
    cleanup.start(QString(HELPERPATH) + "/cleanup", cleanArgs);
    cleanup.waitForFinished();
    listOfIps.removeAll(ip);
}

void UnixSignalHandler::termSignalHandler(int) {
    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    emit u->exiting();
}

void UnixSignalHandler::doExit() {
    static QMutex mutex;
    mutex.lock(); // no need to unlock we exit
    foreach (QProcess* p, listOfProcessToKill) {
        qDebug() << "Closing process";
        if (p) {
            if ((p->state() != QProcess::NotRunning)) {
                qDebug() << p->state();
                p->terminate();
                p->waitForFinished(200);
                if (p->state() != QProcess::NotRunning) {
                    p->kill();
                }
            }
        }
    }

    foreach (Proxy* p, Proxy::proxyHashes.values()) {
        qDebug() << "Delete proxy";
        delete p;
    }

    foreach (QString ip, listOfIps) {
        // cleanup listen Ip
        removeIp(ip);
    }

    _exit(0);
}
