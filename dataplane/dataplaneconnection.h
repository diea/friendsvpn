#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include "abstractplaneconnection.h"
#include "dataplaneclient.h"
#include "dataplaneserver.h"
#include "serverworker.h"

struct dpHeader {
    quint8 sockType;
    quint8 length;
    char md5[33]; // 32 + 1 for '\0'
    char srcIp[INET6_ADDRSTRLEN];
} __attribute__((__packed__));

class DataPlaneConnection : public AbstractPlaneConnection
{
    Q_OBJECT
private:
    QString friendUid; // connection associated with friendUid
    DataPlaneClient* client;
    ServerWorker* server;

    QMutex mutex;

    void removeConnection();
public:
    explicit DataPlaneConnection(QString uid, AbstractPlaneConnection *parent = 0);

    bool addMode(plane_mode, QObject* socket);

    void sendBytes(const char* buf, int len, QString& hash, int sockType, QString& srcIp);
signals:

public slots:
    void readBuffer(const char* buf, int len);

};

#endif // DATAPLANECONNECTION_H
