#ifndef CONTROLPLANECLIENT_H
#define CONTROLPLANECLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QHostAddress>
#include <QSslCertificate>
#include <QSslKey>

class ControlPlaneClient: public QObject
{
    Q_OBJECT
private:
    QSslSocket* sslClient;

    QHostAddress addr;
    int port;

public:
    ControlPlaneClient(QSslCertificate servCert, QSslKey myKey, QHostAddress addr,
                       int port, QObject* parent = 0);
    ~ControlPlaneClient();
private slots:
    /**
     * @brief connectionReady is entered when the SSL handshake has succeeded
     */
    void connectionReady();
public slots:
    void run();
};

#endif // CONTROLPLANECLIENT_H
