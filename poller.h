#ifndef POLLER_H
#define POLLER_H

#include <QObject>
#include "libmaia/xmlRPCTextServer.h"
#include "bonjoursql.h"

/**
 * @brief The Poller class is responsible to poll the web interface to see
 * if any action is required.
 */
class Poller : public QObject
{
    Q_OBJECT
private:
    XmlRPCTextServer* server;
    BonjourSQL* qSql;


public:
    explicit Poller(QObject *parent = 0);

signals:

private slots:
    void fetchXmlRPC();
    void run();
public slots:
    bool setUid(QString uid);
};

#endif // POLLER_H
