#ifndef SSLSERVERTHREAD_H
#define SSLSERVERTHREAD_H

#include <QObject>
#include <QThread>
#include <QSslConfiguration>

class SslServerThread : public QThread
{
    Q_OBJECT
private:
    qintptr sockfd;
    QSslConfiguration sslConfig;
public:
    explicit SslServerThread(qintptr sockfd, QSslConfiguration sslConfig, QObject *parent = 0);

    void run();
signals:

public slots:

};


#endif // SSLSERVERTHREAD_H
