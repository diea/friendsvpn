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
    void startServerEncryption();
    void startClientEncryption();
    ControlPlaneConnection* getControlPlaneConnection();

    /**
     * @brief isAssociated
     * @return TRUE if this SslSocket is associated with a ControlPlaneConnection.
     */
    bool isAssociated();

    void write(const char* buf, int size);
    int read(char* buf, int maxBytes);

    void close();
signals:
    void encrypted();
    void readyRead();
    void disconnected();
private slots:
    void emitRead(int);
public slots:

};

#endif // SSLSOCKET_H
