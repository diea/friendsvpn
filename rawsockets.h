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
    int linkType;
};

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

    void writeBytes(QString srcIp, QString dstIp, int srcPort, const char *transAndPayload, int sockType, int packet_send_size);

signals:

public slots:

};

#endif // RAWSOCKETS_H
