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

    QString friendUid;

    QProcess pcap;
    QProcess sendRaw;
    /**
     * @brief buffer used to buffer "left" bytes until packet has been read
     */
    unsigned int left;
    QByteArray buffer;

    /**
     * @brief con the associated connection
     */
    DataPlaneConnection* con;

    /**
     * @brief defaultIface contains the default interface
     */
    static QString defaultIface;
    static char intToHexChar(int i);
public:
    /**
     * @brief Proxy
     * @param friendUid: the friend for which this proxy is made, so we know where to send the captured data
     *
     * These would be the parameters corresponding to a Bonjour Record, a new one will be made
     * along with a new IP for this proxy:
     * @param name
     * @param regType
     * @param domain
     * @param hostname
     * @param port
     *
     *
     * @param parent
     */
    explicit Proxy(const QString &friendUid, const QString &name, const QString &regType, const QString &domain,
                   const QString &hostname, quint16 port, QObject *parent = 0);

    /**
     * @brief run: runs this proxy
     */
    void run();

    static QString getDefaultInterface();
    static QString randomULA();
signals:

private slots:
    void pcapFinish(int exitCode);
    void sendRawFinish(int exitCode);
    void readyRead();
public slots:

};

#endif // PROXY_H
