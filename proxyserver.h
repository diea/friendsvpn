#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include "proxy.h"
#include <QObject>
#include <QHash>
#include <QProcess>
#include "rawsockets.h"
#include "bonjour/bonjourregistrar.h"

class ProxyServer : public Proxy
{
    Q_OBJECT
private:
    BonjourRecord rec;
    RawSockets* rawSocks;

    QString friendUid;
    BonjourRegistrar registrar;

public:
    explicit ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                        const QString &hostname, quint16 port, const QString& md5);
    void receiveBytes(const char* buf, int len, int sockType, QString& srcIp);

public slots:
    void run();
    void sendBytes(const char *buf, int len, QString srcIp);
};

#endif // PROXYSERVER_H
