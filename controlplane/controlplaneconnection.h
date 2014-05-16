#ifndef CONTROLPLANECONNECTION_H
#define CONTROLPLANECONNECTION_H

#include <QObject>
#include <QSslSocket>
#include "bonjoursql.h"
#include "abstractplaneconnection.h"

class ControlPlaneConnection : public AbstractPlaneConnection
{
    Q_OBJECT
private:
    QSslSocket* serverSock;
    QSslSocket* clientSock;

    BonjourSQL* qSql;

    void removeConnection();
public:
    explicit ControlPlaneConnection(QString uid, AbstractPlaneConnection *parent = 0);
    ~ControlPlaneConnection();

    /**
     * @brief addMode will add the mode to the connection client
     * @param mode must be either Server_mode or Client_mode
     * @param socket (must be of type QSslSocket)
     * returns false on error
     */
    bool addMode(plane_mode mode, QObject* socket);

    /**
     * @brief readBuffer will read a buffer received by the server or client
     * @param buf
     */
    void readBuffer(const char* buf, int len);
signals:
    /**
     * @brief uid_received emitted when the connection leaves the Wait_uid plane_state
     */
    void uid_received();
private slots:
    void wasDisconnected();
public slots:
    void sendBonjour();
};



#endif // CONTROLPLANECONNECTION_H
