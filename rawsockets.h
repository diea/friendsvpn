#ifndef RAWSOCKETS_H
#define RAWSOCKETS_H

#include <QObject>
#include <QProcess>
#include <QHash>
#include <QDebug>
#include "helpers/raw_structs.h"
#include "config.h"
#include "ipresolver.h"

struct rawProcess {
    QProcess* process;
    u_char mac[ETHER_ADDR_LEN];
    int linkType; /* DLT_EN10MB or DLT_NULL */
}; /* represents a raw socket helper */

class RawSockets : public QObject
{
    Q_OBJECT
private:
    static RawSockets* instance;

    QHash<QString, struct rawProcess*> rawHelpers;

    QMutex write;

    /**
     * @brief initializes the raw helpers, one per interface
     */
    explicit RawSockets(QObject *parent = 0);
public:
    static RawSockets* getInstance();

    /**
     * @brief writeBytes will inject the transAndPayload packet using the source IP, destination IP
     * and sockType in the IPv6 header fields
     * @param srcIp
     * @param dstIp
     * @param srcPort
     * @param transAndPayload
     * @param sockType
     * @param packet_send_size
     */
    void writeBytes(QString srcIp, QString dstIp, int srcPort, const char *transAndPayload, int sockType, int packet_send_size);
};

#endif // RAWSOCKETS_H
