#include "unixsignalhandler.h"
#include "ipresolver.h"
#include "config.h"
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
    //setup_unix_signal_handlers();
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
    listOfIps.removeAll(ip);
}

void UnixSignalHandler::termSignalHandler(int) {
    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    static QMutex mutex;
    mutex.lock(); // no need to unlock we exit
    emit u->exiting();
    foreach (QProcess* p, u->listOfProcessToKill) {
        if (p) {
            if ((p->state() != QProcess::NotRunning)) {
                qDebug() << p->state();
                qDebug() << "Closing " << p->pid();
                p->terminate();
                p->waitForFinished(200);
                if (p->state() != QProcess::NotRunning) {
                    qDebug() << "Killing process";
                    p->kill();
                }
            }
        }
    }

    foreach (QString ip, u->listOfIps) {
        qDebug() << "Cleaning up ip" << ip;
        // cleanup listen Ip
        QProcess cleanup;
        QStringList cleanArgs;
        cleanArgs.append(IpResolver::getDefaultInterface());
        cleanArgs.append(ip);
        cleanup.start(QString(HELPERPATH) + "/cleanup", cleanArgs);
        qDebug() << QString(HELPERPATH) + "/cleanup";
        cleanup.waitForStarted();
        cleanup.waitForFinished();
    }

    _exit(0);
}
