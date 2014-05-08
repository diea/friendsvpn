#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include "proxy.h"
#include <QObject>
#include <QHash>
class ProxyServer : public Proxy
{
    Q_OBJECT
private:
    BonjourRecord rec;

    QString friendUid;

    static QString computeMd5(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                           const QString &hostname, quint16 port);

public:
    explicit ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                        const QString &hostname, quint16 port);

signals:
public slots:
    void run();
};

#endif // PROXYSERVER_H
