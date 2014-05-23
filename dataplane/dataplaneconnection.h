#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include "abstractplaneconnection.h"
#include "dataplaneclient.h"
#include "dataplaneserver.h"
#include "serverworker.h"
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

class ProxyClient;

struct dpHeader {
    quint8 sockType;
    quint8 empty;
    quint16 len;
    char md5[16];
    struct in6_addr srcIp;
} __attribute__((__packed__));

class DataPlaneConnection : public AbstractPlaneConnection
{
    Q_OBJECT
private:
    //QString friendUid; // connection associated with friendUid
    DataPlaneClient* client;
    ServerWorker* server;
    QMutex mutex;

    /**
     * @brief lastRcvdTimestap contains the timestamp of the last received packet
     */
    uint lastRcvdTimestamp;

    /**
     * @brief clientProxys contains the list of pointers of proxy clients for this connection
     */
    QStack<ProxyClient*> clientProxys;

    void removeConnection();

    void sendPacket(const char* buf, int len);
public:
    explicit DataPlaneConnection(QString uid, AbstractPlaneConnection *parent = 0);

    bool addMode(plane_mode, QObject* socket);

    void sendBytes(const char* buf, int len, QByteArray& hash, int sockType, QString& srcIp);
public slots:
    void readBuffer(const char* buf, int len);

    void disconnect();

};

#endif // DATAPLANECONNECTION_H
