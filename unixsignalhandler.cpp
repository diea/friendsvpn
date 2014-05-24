#include "unixsignalhandler.h"
#include <signal.h>
#include <QDebug>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <QThread>
#include <unistd.h>

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
    qDebug() << "adding q process and got into mutex";
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
    emit u->exiting();
    qDebug() << "got instance and there are " << u->listOfProcessToKill.length() << "processes to kill";
    foreach (QProcess* p, u->listOfProcessToKill) {
        qDebug() << "Going once more in loop";
        if (p) {
            if ((p->state() != QProcess::NotRunning)) {
                qDebug() << p->state();
                qDebug() << "closing " << p->pid();
                ::kill(p->pid(), SIGTERM);
                p->setProcessState(QProcess::NotRunning);
                waitpid(p->pid(), NULL, WNOHANG);
            }
        }
    }
    qDebug() << "exit!";
    _exit(0);
}
