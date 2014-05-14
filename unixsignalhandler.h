#ifndef UNIXSIGNALHANDLER_H
#define UNIXSIGNALHANDLER_H

#include <QObject>
#include <QProcess>
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
    explicit UnixSignalHandler(QObject *parent = 0);

    static UnixSignalHandler* instance;
public:
    static UnixSignalHandler* getInstance();

    /**
     * @brief addQProcess adds a QProcess to close before shutting down
     * @param p
     */
    void addQProcess(QProcess* p);

    static void termSignalHandler(int unused);
};

#endif // UNIXSIGNALHANDLER_H
