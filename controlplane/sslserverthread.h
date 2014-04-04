#ifndef SSLSERVERTHREAD_H
#define SSLSERVERTHREAD_H

#include <QObject>
#include <QThread>
#include <QSslConfiguration>

class SslServerThread : public QThread
{
    Q_OBJECT
private:
    int sockfd;
    QSslConfiguration sslConfig;
public:
    explicit SslServerThread(int sockfd, QSslConfiguration sslConfig, QObject *parent = 0);

    void run();
signals:

public slots:

};

#endif // SSLSERVERTHREAD_H
