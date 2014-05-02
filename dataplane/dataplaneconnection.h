#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include "abstractplaneconnection.h"
#include "dataplaneclient.h"
#include "dataplaneserver.h"
#include "serverworker.h"

class DataPlaneConnection : public AbstractPlaneConnection
{
    Q_OBJECT
private:
    QString friendUid; // connection associated with friendUid
    DataPlaneClient* client;
    ServerWorker* server;

    void removeConnection();
public:
    explicit DataPlaneConnection(QString uid, AbstractPlaneConnection *parent = 0);

    bool addMode(plane_mode, QObject* socket);

    void readBuffer(const char* buf);

    void sendBytes(const char* buf, int len);
signals:

public slots:

};

#endif // DATAPLANECONNECTION_H
