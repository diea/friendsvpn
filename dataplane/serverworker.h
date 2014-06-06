#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QSocketNotifier>
#include <QMutex>
#include "dataplaneconfig.h"

class DataPlaneConnection;

class ServerWorker : public QObject
{
    Q_OBJECT
private:
    addrUnion server_addr, client_addr;
    SSL *ssl;
    int fd;
    DataPlaneConnection *con;

    QSocketNotifier* notif;

    QMutex closeProtect; // protect close procedure to prevent SIGSEGV in read
public:
    ~ServerWorker();
    explicit ServerWorker(addrUnion server_addr, addrUnion client_addr, SSL* ssl, DataPlaneConnection* con, QObject *parent = 0);
    void stop();
    void sendBytes(const char* buf, int len);
signals:
    /**
     * @brief bufferReady used to signal that buffer buf has been read
     * @deprecated not used, direct reference to the dp connection is used to gain speed
     * @param buf
     * @param len
     */
    void bufferReady(const char* buf, int len);
private slots:
    /**
     * @brief readyRead bytes are ready on the data plane connection
     * @param fd
     */
    void readyRead(int fd);
public slots:
    void connection_handle();
};

#endif // SERVERWORKER_H
