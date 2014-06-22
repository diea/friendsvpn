#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QApplication>
#include <QObject>
#include <QAction>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
/**
 * @brief The SysTray class creates the system tray icon and the available actions for it.
 */
class SysTray : public QWidget
{
    Q_OBJECT
private:
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QAction* sendBjr;
    QAction* quitAction;

    static SysTray* instance;
    SysTray(QWidget *parent = 0);

private slots:
    void createActions();
    void createTrayIcon();
    void quit();
    void sendBjrSlot();
public slots:
    void run();
public:
    static SysTray* getInstance();
    void closeEvent(QCloseEvent *event);
    QAction* getSendBonjour();
signals:
    void sendBonjour();
};

#endif // SYSTRAY_H
