#ifndef ABSTRACTPLANECONNECTION_H
#define ABSTRACTPLANECONNECTION_H

#include <QObject>

/**
 * @brief The plane_mode enum is used by a PlaneConnection to know if it is the emitter or the
 * receiver.
 */
enum plane_mode { Closed, Emitting, Receiving, Both };

class AbstractPlaneConnection : public QObject
{
    Q_OBJECT
protected:
    plane_mode curMode;
    /**
     * @brief friendUid is the uid of the friend with which the connection is made.
     */
    QString friendUid;

    void setMode(plane_mode m);

    /**
     * @brief removeConnection is called when the ControlPlaneConnection arrives in state "Both_mode"
     * It will compare the user's UID with the friends UID and close a connection accordingly.
     */
    virtual void removeConnection() = 0;

    explicit AbstractPlaneConnection(QString uid, QObject *parent = 0);
public:
    plane_mode getMode();

    /**
     * @brief addMode will add the mode to the connection client
     * @param mode must be either Server_mode or Client_mode
     * @param socket
     * returns false on error
     */
    virtual bool addMode(plane_mode, QObject* socket) = 0;
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
    virtual void readBuffer(const char* buf) = 0;

    /**
     * @brief operator = Two ControlPlaneConnections are identical if they represent the same UID.
     * /!\ They could be in different states and still return equality with this function. But if you
     * have two ControlPlaneConnections for the same UID and in different states, you're doing it wrong.
     * @param other
     * @return
     */
    bool operator=(const AbstractPlaneConnection& other);
signals:
    /**
     * @brief connected emitted when the connection leaves the Closed plane_mode
     */
    void connected();
    /**
     * @brief disconnected emitted when the connection enters the Closed plane_mode
     */
    void disconnected();
public slots:

};

#endif // ABSTRACTPLANECONNECTION_H
