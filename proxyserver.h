#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include "proxy.h"
#include <QObject>
#include <QHash>
#include <QProcess>

class ProxyServer : public Proxy
{
    Q_OBJECT
private:
    BonjourRecord rec;

    QString friendUid;
    QMutex mutex;

    /**
     * @brief clients we maintain a list of raw sockets, one per client
     */
    QHash<QString, QProcess*> clients;

    //void receiveBytes(char *buf, int len, int sizeLen, QString& hash, int sockType, QString& srcIp);

    static QString computeMd5(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                           const QString &hostname, quint16 port);

public:
    explicit ProxyServer(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                        const QString &hostname, quint16 port);
    void receiveBytes(const char* buf, int len, int sockType, QString& srcIp);

signals:
    void newRaw(QString dstIp, int dstPort);
private slots:
    void serverNewRaw(QString dstIp, int dstPort);
public slots:
    void run();
    void sendBytes(const char *buf, int len, QString srcIp);
};

#endif // PROXYSERVER_H
