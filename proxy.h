#ifndef PROXY_H
#define PROXY_H

#include <QObject>
/**
 * @brief The Proxy class
 * Generates a new IPv6 for this host, then registers an associated bonjour service
 * It will create an IP_SOCKRAW for this service and forward the [TransportHeader|Payload] on
 * the secure UDP connection to the corresponding friend
 */
class Proxy : public QObject
{
    Q_OBJECT
public:
    explicit Proxy(QObject *parent = 0);

signals:

public slots:

};

#endif // PROXY_H
