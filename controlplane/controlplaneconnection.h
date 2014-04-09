#ifndef CONTROLPLANECONNECTION_H
#define CONTROLPLANECONNECTION_H

#include <QObject>
#include <QSslSocket>
#include "bonjoursql.h"
enum plane_mode { Closed, Server_mode, Client_mode,
       Both_mode };

class ControlPlaneConnection : public QObject
{
    Q_OBJECT
private:
    plane_mode curMode;
    QSslSocket* serverSock;
    QSslSocket* clientSock;
    /**
     * @brief friendUid is the uid of the friend with which the connection is made.
     */
    QString friendUid;
    BonjourSQL* qSql;

    void setMode(plane_mode m);

    /**
     * @brief removeConnection is called when the ControlPlaneConnection arrives in state "Both_mode"
     * It will compare the user's UID with the friends UID and close a connection accordingly.
     */
    void removeConnection();
public:
    explicit ControlPlaneConnection(QString uid, QObject *parent = 0);
    ~ControlPlaneConnection();
    QString getUid();

    plane_mode getMode();

    /**
     * @brief addMode will add the mode to the connection client
     * @param mode must be either Server_mode or Client_mode
     * @param socket
     * returns false on error
     */
    bool addMode(plane_mode mode, QSslSocket* socket);
    /**
     * @brief addMode will remove the mode to the connection client
     * @param mode must be either Server_mode or Client_mode
     * returns false on error
     */
    bool removeMode(plane_mode mode);


    /**
     * @brief readBuffer will read a buffer received by the server or client
     * @param buf
     */
    void readBuffer(const char* buf);
    /**
     * @brief operator = Two ControlPlaneConnections are identical if they represent the same UID.
     * /!\ They could be in different states and still return equality with this function. But if you
     * have two ControlPlaneConnections for the same UID and in different states, you're doing it wrong.
     * @param other
     * @return
     */
    bool operator=(const ControlPlaneConnection& other);
signals:
    /**
     * @brief connected emitted when the connection leaves the Closed plane_mode
     */
    void connected();
    /**
     * @brief disconnected emitted when the connection enters the Closed plane_mode
     */
    void disconnected();
    /**
     * @brief uid_received emitted when the connection leaves the Wait_uid plane_state
     */
    void uid_received();
public slots:
    void sendBonjour();
};



#endif // CONTROLPLANECONNECTION_H
