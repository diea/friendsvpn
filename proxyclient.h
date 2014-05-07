#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H
#include "proxy.h"

class ProxyClient : public Proxy
{
    //Q_OBJECT
private:
    QString listenIp;
    int listenPort;
    /**
     * @brief buffer used to buffer "left" bytes until packet has been read
     */
    unsigned int left;
    QByteArray buffer;

    QString idHash;

    /**
     * @brief proxyHashes each proxy will be identified by a hash of its constructor parameters
     * so that we don't create twice the same proxy
     */
    static QHash<QString, ProxyClient*> proxyHashes;


    ProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);
public:
    ProxyClient* getProxyClient(QString md5, int sockType, int srcPort, DataPlaneConnection* con);

    void sendBytes(const char* buf, int len);

public slots:
    void run();
private slots:
    void pcapFinish(int exitCode);
    //void sendRawFinish(int exitCode);
    void readyRead();

};

#endif // PROXYCLIENT_H
