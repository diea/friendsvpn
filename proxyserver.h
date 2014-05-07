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
    QString idHash;

    /**
     * @brief buffer used to buffer "left" bytes until packet has been read
     */
    unsigned int left;
    QByteArray buffer;

    QString friendUid;

    /**
     * @brief proxyHashes each proxy will be identified by a hash of its constructor parameters
     * so that we don't create twice the same proxy
     */
    static QHash<QString, ProxyServer*> proxyHashes;
public:
    explicit ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                        const QString &hostname, quint16 port);

signals:
private slots:
    void pcapFinish(int exitCode);
    void sendRawFinish(int exitCode);
    void readyRead();
public slots:
    void run();
};

#endif // PROXYSERVER_H
