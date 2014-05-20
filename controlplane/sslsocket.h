#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#include "controlplaneconnection.h"
/**
 * @brief The SslSocket class inherits the QSslSocket class to be able to be associated with
 * a ControlPlaneConnection.
 */
class SslSocket : public QObject
{
    Q_OBJECT
private:
    ControlPlaneConnection* con;

    SSL* ssl;
    int sd;
    QSocketNotifier* notif;
public:
    explicit SslSocket(SSL* ssl, QObject *parent = 0);
    ~SslSocket();
    void setControlPlaneConnection(ControlPlaneConnection* con);

    ControlPlaneConnection* getControlPlaneConnection();

    /**
     * @brief isAssociated
     * @return TRUE if this SslSocket is associated with a ControlPlaneConnection.
     */
    bool isAssociated();
signals:

public slots:

};

#endif // SSLSOCKET_H
