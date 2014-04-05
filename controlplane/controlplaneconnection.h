#ifndef CONTROLPLANECONNECTION_H
#define CONTROLPLANECONNECTION_H

#include <QObject>

enum plane_mode { Closed, Server_mode, Client_mode,
       Both_mode };
enum plane_state { Wait_uid, Wait_data };

class ControlPlaneConnection : public QObject
{
    Q_OBJECT
private:
    plane_mode curMode;
    plane_state curState;

    /**
     * @brief friendUid is the uid of the friend with which the connection is made.
     */
    QString friendUid;

    void setMode(plane_mode m);
public:
    explicit ControlPlaneConnection(QObject *parent = 0);

    void setState(plane_state s);
    /**
     * @brief setUid sets the uid of the friend for which this connection is.
     * @param friendUid
     */
    void setUid(QString friendUid);
    QString getUid();

    plane_mode getMode();
    plane_state getState();
    void receiveBonjour();

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
};



#endif // CONTROLPLANECONNECTION_H
