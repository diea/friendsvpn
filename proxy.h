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
#include "databasehandler.h"
#include "dataplane/dataplaneconnection.h"
#include "ipresolver.h"
#include "rawsockets.h"

#include "pcapworker.h"

struct prefix {
    QString str;
    int len;
};

/**
 * @brief The Proxy class acts as an abstract class that can is extended by the ProxyServer
 * and ProxyClient to proxy a service (as a server or as a client).
 */
class Proxy : public QObject
{
    Q_OBJECT
    friend class PcapWorker;
private:
    static IpResolver* resolver;
    static RawSockets* rawSockets;

    QStack<PcapWorker*> pcapWorkers;
    QStack<QProcess*> processes; // QProcesses associated with this Proxy

protected:
    QString listenIp; // the new IP on which this proxy listens for answers
    int listenPort; // the prefered listen port

    int port; // the active listen port, which may differ when the listen port was not available
    // the listenPort will then replace the port in the packets sent over the data plane connection

    int sockType; // to know if SOCK_STREAM or SOCK_DATAGRAM
    int ipProto; // again, TCP or UDP

    QByteArray idHash; // the proxy's 128bit long identifier

    /**
     * @brief con the associated connection
     */
    DataPlaneConnection* con;

    /**
     * @brief Proxy is an abstract class
     */
    void commonInit(QByteArray md5); // does the common initialization
    Proxy(int port, int sockType, QByteArray md5);
    Proxy(int port, const QString& regType, QByteArray md5);

    /**
     * @brief proxyHashes each proxy will be identified by a hash of its constructor parameters
     * so that we don't create twice the same proxy
     */
    static QHash<QByteArray, Proxy*> proxyHashes;
    /**
     * @brief defaultIface contains the default interface
     */
    static QString defaultIface;

    static QMutex poolOfIpsMutex;
    static QQueue<QString> poolOfIps; /* IPs which are ready to ``bind" on */

    static char intToHexChar(int i); // transforms an integer (< 16) into its hexadecimal representation

    /**
     * @brief stripIp
     * @param ip
     * @param prefix
     * @return the ip "stripped", meaning the prefix's IP.
     */
    static QString stripIp(QString ip, int prefix);
    /**
     * @brief getPrefix get the default's interface IPv6 prefix
     * @return
     */
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
     * @param dstIp is used by ProxyClient to know on which iface to start the pcap helper
     */
    void run_pcap(const char* dstIp = 0);

    /**
     * @brief receiveBytes
     * @param buf
     * @param len = sizeLen + length of packet
     * @param sizeLen is the size of the [size] string in the packet buffer
     * @param hash
     * @param sockType
     * @param srcIp
     */
    virtual void receiveBytes(const char* buf, int len, int sockType, QString srcIp) = 0;
public:
    ~Proxy();
    /**
     * @brief gennewIP generates a new random IP, adds it to the kernel and adds it to our pool
     * of ips. We use such a pool since the kernel needs a bit of time to accomodate this new IP
     * and we don't want un-necessary delays when making a new incoming proxy client!
     * @return
     */
    static void gennewIP();

    /**
     * @brief getProxy static method that returns the Proxy* identified by the md5
     * @param md5
     * @return
     */
    static Proxy* getProxy(QByteArray md5);

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
