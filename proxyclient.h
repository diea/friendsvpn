#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H
#include "bonjour/bonjourrecord.h"
#include "proxy.h"

class RawSockets;
/**
 * @brief The ProxyClient class acts as the client for the local service by injecting and capturing
 * packets for and from the distant service client.
 */
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

    /**
     * @brief receiveBytes pcap has captured a response which should be sent over the data plane
     * connection
     * @param buf
     * @param len
     * @param sockType
     * @param srcIp
     */
    void receiveBytes(const char* buf, int len, int sockType, QString srcIp);

private slots:
    void timeout(); // called when QTimer times out
public slots:
    void run();
    /**
     * @brief sendBytes injects the bytes buf and len on the local network
     * @param buf
     * @param len
     * @param srcIp not used by ProxyClient, it's used by the ProxyServer to know to which client to send
     */
    void sendBytes(const char *buf, int len, QString srcIp, quint8 fragType);

};

#endif // PROXYCLIENT_H
