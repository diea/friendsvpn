#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>

#ifdef __APPLE__
#define HELPERPATH "/Users/diea/Dropbox/CiscoVPN/app/friendsvpn/helpers/"
#elif __GNUC__
#define HELPERPATH "./helpers/"
#endif

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
    int sockType; // to know if SOCK_STREAM or SOCK_DATAGRAM
    int ipProto; // again, TCP or UDP

    QProcess pcap;
    QProcess sendRaw;
    /**
     * @brief buffer used to buffer "left" bytes until packet has been read
     */
    unsigned int left;
    QByteArray buffer;
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

private slots:
    void pcapFinish(int exitCode);
    void sendRawFinish(int exitCode);
    void readyRead();
public slots:

};

#endif // PROXY_H
