#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H
#include "bonjour/bonjourrecord.h"
#include "proxy.h"

class RawSockets;

class ProxyClient : public Proxy
{
    Q_OBJECT
private:
    /**
     * @brief serverRecord will be the server record for which this proxy client will proxy to
     */
    BonjourRecord* serverRecord;
    QString servermd5;
    QString serversrcIp; /* the ip that the server will have to fwd (the real client of the server) */
    RawSockets* rawSocks;
    int origSrcPort;

public:
    ProxyClient(QString md5, QString servermd5, QString serversrcIp, int sockType, int srcPort, DataPlaneConnection* con);

    //ProxyClient* getProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);
    void receiveBytes(const char* buf, int len, int sockType, QString& srcIp);

signals:

public slots:
    void run();
    void sendBytes(const char *buf, int len, QString srcIp);

};

#endif // PROXYCLIENT_H
