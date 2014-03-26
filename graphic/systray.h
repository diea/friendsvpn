#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QApplication>
#include <QObject>
#include <QAction>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>

class SysTray : public QWidget
{
    Q_OBJECT
public:
    explicit SysTray(QWidget *parent = 0);
    void closeEvent(QCloseEvent *event);
    void createActions();
    void createTrayIcon();
private:
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QAction* restoreAction;
    QAction* quitAction;
signals:

public slots:

};

#endif // SYSTRAY_H
