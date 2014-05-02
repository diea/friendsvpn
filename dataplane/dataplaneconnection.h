#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include "dataplaneclient.h"
#include "dataplaneserver.h"

class DataPlaneConnection : public QObject
{
    Q_OBJECT
private:
    DataPlaneClient* client;
    DataPlaneServer* server;

public:
    explicit DataPlaneConnection(QObject *parent = 0);

signals:

public slots:

};

#endif // DATAPLANECONNECTION_H
