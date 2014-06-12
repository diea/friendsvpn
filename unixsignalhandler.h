#ifndef UNIXSIGNALHANDLER_H
#define UNIXSIGNALHANDLER_H

#include <QObject>
#include <QProcess>
#include <QMutex>
#include <QMap>
/**
 * @brief The UnixSignalHandler class will handle the SIGTERM and SIGINT signals to clearup things
 * that the app must clean before exiting.
 *
 * Inspired by http://qt-project.org/doc/qt-4.8/unix-signals.html and the man 2 sigaction
 */
class UnixSignalHandler : public QObject
{
    Q_OBJECT
private:
    /**
     * @brief setup_unix_signal_handlers init by using the sigaction system call to designate
     * termSignalHandler as the handler for both those signals
     * @return 0 on success and > 0 on failure
     */
    static int setup_unix_signal_handlers();

    QList<QProcess*> listOfProcessToKill;
    QList<QThread*> listOfThreads;
    QList<QString> listOfIps;
    explicit UnixSignalHandler(QObject *parent = 0);

    static UnixSignalHandler* instance;
public:
    static UnixSignalHandler* getInstance();

    /**
     * @brief addQProcess adds a QProcess to close before shutting down
     * @param p
     */
    void addQProcess(QProcess* p);
    void removeQProcess(QProcess *p);

    /**
     * @brief addIp add ip associated with the ifconfighelp process p
     * @param ip
     * @param p
     */
    void addIp(QString ip);
    void removeIp(QString ip);

    static void termSignalHandler(int unused);
signals:
    void exiting();
public slots:
    void doExit();
};

#endif // UNIXSIGNALHANDLER_H
