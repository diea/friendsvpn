#ifndef SSLSOCKET_H
#define SSLSOCKET_H

#include "controlplaneconnection.h"
#define SSL_BUFFERSIZE 32768
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

    /**
     * @brief buffer will buffer the multiple reads until a \r\n\r\n is present
     */
    QMutex mutex;
    char buf[SSL_BUFFERSIZE]; /* 256kb buffer */
    int bytesRead;
    int nextFullPacket;
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

    /**
     * @brief read
     * @param buf should be at least 256K
     * @return
     */
    int read(char* buf);

    void close();
signals:
    void encrypted();
    void readyRead();
    void disconnected();
private slots:
    void getNewBytes();
public slots:

};

#endif // SSLSOCKET_H
