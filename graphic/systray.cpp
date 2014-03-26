#include "systray.h"
#include <QDebug>

SysTray::SysTray(QWidget *parent) :
    QWidget(parent)
{
    createActions();
    createTrayIcon();
    qDebug() << "got here";
}

void SysTray::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        trayIcon->hide();
        event->ignore();
    }
}

void SysTray::createActions()
{
    restoreAction = new QAction(tr("&Show"), this);
    this->connect(restoreAction, SIGNAL(triggered()), this, SLOT(show()));

    quitAction = new QAction(tr("&Exit"), this);
    this->connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void SysTray::createTrayIcon()
{
    //QIcon icon = QIcon("/Users/diea/Dropbox/CiscoVPN/app/friendsvpn/favicon.png");
    QIcon icon = QIcon(":images/favicon.png");
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(icon);
    trayIcon->show();
}
