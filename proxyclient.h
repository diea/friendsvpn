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
    QByteArray servermd5;
    QString serversrcIp; /* the ip that the server will have to fwd (the real client of the server) */
    RawSockets* rawSocks;
    int origSrcPort;

    QTimer timer;

public:
    ProxyClient(QByteArray md5, QByteArray servermd5, QString serversrcIp, int sockType, int srcPort, DataPlaneConnection* con);

    //ProxyClient* getProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);
    void receiveBytes(const char* buf, int len, int sockType, QString& srcIp);

signals:
private slots:
    void timeout(); // called when QTimer times out
public slots:
    void run();
    void sendBytes(const char *buf, int len, QString srcIp);

};

#endif // PROXYCLIENT_H
