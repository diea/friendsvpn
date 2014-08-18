#include "systray.h"
#include "unixsignalhandler.h"
#include <QDebug>
#include <QMutex>

SysTray* SysTray::instance = NULL;

SysTray::SysTray(QWidget *parent) :
    QWidget(parent)
{
}

SysTray* SysTray::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (instance == NULL) {
        instance = new SysTray();
    }
    mutex.unlock();
    return instance;
}

void SysTray::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        trayIcon->hide();
        event->ignore();
    }
}

QAction* SysTray::getSendBonjour() {
    return sendBjr;
}

void SysTray::run() {
    createActions();
    createTrayIcon();
}

void SysTray::createActions()
{
    sendBjr = new QAction(tr("&Send bonjour"), this);
    quitAction = new QAction(tr("&Exit"), this);
    this->connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));
    this->connect(sendBjr, SIGNAL(triggered()), this, SLOT(sendBjrSlot()));
}

void SysTray::sendBjrSlot() {
    emit sendBonjour();
}

void SysTray::quit() {
    UnixSignalHandler::termSignalHandler(0);
}

void SysTray::createTrayIcon()
{
    QIcon icon = QIcon(":images/favicon.png");
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(sendBjr);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(icon);
    trayIcon->show();
}
