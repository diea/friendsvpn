#ifndef POLLER_H
#define POLLER_H

#include <QObject>
#include "libmaia/xmlRPCTextServer.h"
#include "databasehandler.h"

/**
 * @brief The Poller class is responsible to poll the web interface to see
 * if any action is required.
 */
class Poller : public QObject
{
    Q_OBJECT
private:
    XmlRPCTextServer* server;
    DatabaseHandler* qSql;

    static Poller* instance;

    explicit Poller(QObject *parent = 0);

public:
    static Poller* getInstance();

private slots:
    /**
     * @brief fetchXmlRPC fetches the last XMLRPC request from the database
     */
    void fetchXmlRPC();
    void run();
public slots:
    /**
     * @brief setUid is an available XMLRPC method that the web interface can access using XMLRPC.
     * This is used to set the uid of the application user.
     * @param uid
     * @return
     */
    bool setUid(QString uid);
    /**
     * @brief bonjourChanged XMLRPC when bonjour record has changed, emits bonjourChanged
     */
    void emitBonjourChanged();
signals:
    void bonjourChanged();
};

#endif // POLLER_H
