#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include "abstractplaneconnection.h"
#include "dataplaneclient.h"
#include "dataplaneserver.h"
#include "serverworker.h"
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

class ProxyClient;

/**
 * @brief The dpHeader struct is the PH2PHTP data plane custom header.
 */
struct dpHeader {
    quint8 sockType; /* underlying transport header socket type */
    quint8 fragType; /* 0 if not a frag, 1 if is first frag packet, 2 if not first frag, 3 if the last */
    quint16 len; /* underlying packet length (including transport header) */
    char md5[16]; /* MD5 identifying ProxyServer */
    struct in6_addr srcIp; /* source IP of the client using the service */
} __attribute__((__packed__));

struct dpFragHeader {
    quint32 fragId;
    quint32 offset; /* offset for fragmented packet, in number of bytes */
    quint16 offsetLen;
} __attribute__((__packed__));

/**
 * @brief The DataPlaneConnection class
 */
class DataPlaneConnection : public AbstractPlaneConnection
{
    Q_OBJECT
private:
    DataPlaneClient* client;
    ServerWorker* server;
    QMutex mutex;

    /**
     * @brief globalIdFrag used by the dataplaneconnection to fragment outgoing fragments
     */
    quint32 globalIdFrag;
    QMutex globalIdMutex;

    /**
     * @brief fragmentBuffer will hold the different fragment bits for each incoming packet that
     * needs to be re-assembled.
     */
    QHash<quint32, char*> fragmentBuffer;
    /**
     * @brief remainingBits used in parallel with the fragment buffer, tells how many bits are
     * waiting to be received
     * a qint32 is used to be able to go under 0 and prevent a wraparound which would cause segfault
     */
    QHash<quint32, qint32> remainingBits;
    /**
     * @brief totalSize also used in parallel with the two preceding, is there to prevent buffer overflow
     */
    QHash<quint32, quint16> totalSize;

    /**
     * @brief lastRcvdTimestap contains the timestamp of the last received packet
     */
    uint lastRcvdTimestamp;

    /**
     * @brief clientProxys contains the list of pointers of proxy clients for this connection
     */
    QStack<ProxyClient*> clientProxys;

    void removeConnection();

    /**
     * @brief sendPacket sends a packet containg buf of length len on the data plane connection
     * @param buf
     * @param len
     */
    void sendPacket(const char* buf, int len);

    /**
     * @brief maxPayloadLen is the maximum payload length for a data plane packets
     * It corresponds to the maximum multiple of 8 that is smaller than
     * IPV6_MIN_MTU - sizeof(struct ether_header) - sizeof(struct ipv6hdr)
     */
    static quint16 maxPayloadLen;
    //static quint16 initMaxPayloadLen();
public:
    explicit DataPlaneConnection(QString uid, AbstractPlaneConnection *parent = 0);

    bool addMode(plane_mode, QObject* socket);

    /**
     * @brief sendBytes sends the buffer of length len by setting the right data plane header
     * @param buf
     * @param len
     * @param hash => unique bit string to be set in the MD5 field of the data plane header
     * @param sockType
     * @param srcIp => client address in dp header
     */
    void sendBytes(const char* buf, int len, QByteArray& hash, int sockType, QString& srcIp);
public slots:
    /**
     * @brief readBuffer reads the buffer coming from a data plane connection
     * @param buf
     * @param len
     */
    void readBuffer(char* buf, int len);

    void disconnect();

};

#endif // DATAPLANECONNECTION_H
