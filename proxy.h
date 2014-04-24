#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include "bonjour/bonjourrecord.h"
#include "bonjoursql.h"

/**
 * @brief The Proxy class
 * Generates a new IPv6 for this host, then registers an associated bonjour service
 * It will create an IP_SOCKRAW for this service and forward the [TransportHeader|Payload] on
 * the secure UDP connection to the corresponding friend
 */
class Proxy : public QObject
{
    Q_OBJECT
private:
    BonjourRecord rec;

    /**
     * @brief randomULA generates a random ULA and returns it as a QString
     * @return
     */
    QString randomULA();
public:
    explicit Proxy(const QString &name, const QString &regType, const QString &domain,
                   const QString &hostname, quint16 port, QObject *parent = 0);

    /**
     * @brief run: runs this proxy
     */
    void run();
signals:

public slots:

};

#endif // PROXY_H
