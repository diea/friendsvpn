#ifndef UNIXSIGNALHANDLER_H
#define UNIXSIGNALHANDLER_H

#include <QObject>
#include <QProcess>

class UnixSignalHandler : public QObject
{
    Q_OBJECT
private:
    static int setup_unix_signal_handlers();

    QList<QProcess*> listOfProcessToKill;
    explicit UnixSignalHandler(QObject *parent = 0);

    static UnixSignalHandler* instance;
public:
    static UnixSignalHandler* getInstance();

    void addQProcess(QProcess* p);

    static void termSignalHandler(int unused);

signals:

public slots:

};

#endif // UNIXSIGNALHANDLER_H
