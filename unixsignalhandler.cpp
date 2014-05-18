#include "unixsignalhandler.h"
#include <signal.h>
#include <QDebug>


UnixSignalHandler* UnixSignalHandler::instance = NULL;
QMutex UnixSignalHandler::mutex;

UnixSignalHandler::UnixSignalHandler(QObject *parent) :
    QObject(parent)
{
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
    mutex.lock();
    listOfProcessToKill.append(p);
    mutex.unlock();
}

void UnixSignalHandler::removeQProcess(QProcess *p) {
    mutex.lock();
    listOfProcessToKill.removeAll(p);
    mutex.unlock();
}

void UnixSignalHandler::termSignalHandler(int) {
    qDebug() << "terminating!";
    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    mutex.lock(); // no need to unlock we exit
    qDebug() << "got instance";
    foreach (QProcess* p, u->listOfProcessToKill) {
        qDebug() << "Going once more in loop";
        if (p) {
            if ((p->state() != QProcess::NotRunning)) {
                qDebug() << p->state();
                qDebug() << "closing";
                p->terminate();
            }
        }
    }
    qDebug() << "exit!";
    _exit(0);
}
