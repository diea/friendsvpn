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
#include "dataplane/dataplaneconnection.h"
#include "ph2phtp_parser.h"

struct prefix {
    QString str;
    int len;
};

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
    QProcess* sendRaw;
    /**
     * @brief initRaw initializes the raw socket to inject packets
     */
    void initRaw();
protected:
    QString listenIp;
    int listenPort;

    int sockType; // to know if SOCK_STREAM or SOCK_DATAGRAM
    int ipProto; // again, TCP or UDP

    QString idHash;

    /**
     * @brief con the associated connection
     */
    DataPlaneConnection* con;

    /**
     * @brief Proxy is an abstract class
     */
    Proxy(int port, int sockType);
    Proxy(int port, const QString& regType);

    /**
     * @brief proxyHashes each proxy will be identified by a hash of its constructor parameters
     * so that we don't create twice the same proxy
     */
    static QHash<QString, Proxy*> proxyHashes;
    /**
     * @brief defaultIface contains the default interface
     */
    static QString defaultIface;

    static char intToHexChar(int i);

    /**
     * @brief stripIp
     * @param ip
     * @param prefix
     * @return the ip "stripped", meaning the prefix's IP.
     */
    static QString stripIp(QString ip, int prefix);
    static struct prefix getPrefix();
    static QString randomIP();

    /**
     * @brief newIP generates a new random IP, adds it to the kernel and returns the new IP as
     * a string
     * @return
     */
    static QString newIP();
public:
    static QString getDefaultInterface();

    static Proxy* getProxy(QString md5);
private slots:
    void sendRawFinish(int);
public slots:
    /**
     * @brief run: runs this proxy
     */
    virtual void run() = 0;

    void sendBytes(const char* buf, int len);
};

#endif // PROXY_H
