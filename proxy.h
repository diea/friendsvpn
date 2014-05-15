#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>

#include "bonjour/bonjourrecord.h"
#include "bonjoursql.h"
#include "dataplane/dataplaneconnection.h"
#include "ph2phtp_parser.h"
#include "ipresolver.h"
#include "rawsockets.h"

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
    static IpResolver* resolver;
    static RawSockets* rawSockets;

protected:
    QString listenIp;
    int listenPort; // the prefered listen port

    int port; // the active listen port

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
    Proxy(int port, int sockType, QString md5);
    Proxy(int port, const QString& regType, QString md5);

    /**
     * @brief proxyHashes each proxy will be identified by a hash of its constructor parameters
     * so that we don't create twice the same proxy
     */
    static QHash<QString, Proxy*> proxyHashes;
    /**
     * @brief defaultIface contains the default interface
     */
    static QString defaultIface;

    static QMutex poolOfIpsMutex;
    static QStringList poolOfIps;

    static char intToHexChar(int i);

    /**
     * @brief stripIp
     * @param ip
     * @param prefix
     * @return the ip "stripped", meaning the prefix's IP.
     */
    static QString stripIp(QString ip, int prefix);
    static struct prefix getPrefix();

    /**
     * @brief randomIP
     * @return a random IP with prefix notation: e.g. fd3b:e180:cbaa::4/48
     */
    static QString randomIP();

    /**
     * @brief takes out an IP from the ip buffer.
     * @return
     */
    static QString newIP();

    /**
     * @brief run_pcap will run the pcap processes for this proxy
     */
    void run_pcap();

    /**
     * @brief receiveBytes
     * @param buf
     * @param len = sizeLen + length of packet
     * @param sizeLen is the size of the [size] string in the packet buffer
     * @param hash
     * @param sockType
     * @param srcIp
     */
    virtual void receiveBytes(const char* buf, int len, int sockType, QString& srcIp) = 0;
public:
    /**
     * @brief gennewIP generates a new random IP, adds it to the kernel and adds it to our pool
     * of ips. We use such a pool since the kernel needs a bit of time to accomodate this new IP
     * and we don't want un-necessary delays when making a new incoming proxy client!
     * @return
     */
    static void gennewIP();
    static QString getDefaultInterface();

    static Proxy* getProxy(QString md5);
private slots:
    void sendRawFinish(int);
    void sendRawStandardError();
    void sendRawStandardOutput();
protected slots:
    void pcapFinish(int exitCode);
    void readyRead();

public slots:
    /**
     * @brief run: runs this proxy
     */
    virtual void run() = 0;

    /**
     * @brief sendBytes
     * @param buf
     * @param len
     * @param dstIp
     *
     * dstIp argument is unused by client, it's for the server to know to which ip to send
     */
    virtual void sendBytes(const char* buf, int len, QString dstIp) = 0;
};

#endif // PROXY_H
