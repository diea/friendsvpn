#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#include <QSslSocket>
#include "controlplaneconnection.h"
/**
 * @brief The SslSocket class inherits the QSslSocket class to be able to be associated with
 * a ControlPlaneConnection.
 */
class SslSocket : public QSslSocket
{
    Q_OBJECT
private:
    ControlPlaneConnection* con;
public:
    explicit SslSocket(QObject *parent = 0);
    ~SslSocket();
    void setControlPlaneConnection(ControlPlaneConnection* con);

    ControlPlaneConnection* getControlPlaneConnection();

    /**
     * @brief isAssociated
     * @return TRUE if this SslSocket is associated with a ControlPlaneConnection.
     */
    bool isAssociated();
};

#endif // SSLSOCKET_H
