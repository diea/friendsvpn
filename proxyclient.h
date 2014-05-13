#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H
#include "proxy.h"
#include "bonjour/bonjourrecord.h"

class ProxyClient : public Proxy
{
    Q_OBJECT
private:
    /**
     * @brief serverRecord will be the server record for which this proxy client will proxy to
     */
    BonjourRecord* serverRecord;
    QProcess* sendRaw;
    QMutex mutex;
    QString servermd5;

    int origSrcPort;

    //void receiveBytes(char *buf, int len, int sizeLen, QString& hash, int sockType, QString& srcIp);

public:
    ProxyClient(QString md5, QString servermd5, int sockType, int srcPort, DataPlaneConnection* con);

    //ProxyClient* getProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);
    //void sendBytes(const char* buf, int len);

signals:

public slots:
    void run();
    void sendBytes(const char *buf, int len, QString srcIp);

};

#endif // PROXYCLIENT_H
