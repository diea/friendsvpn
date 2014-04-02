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
    QSslCertificate servCert;
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

    void sslErrors(const QList<QSslError>& errs);
public slots:
    /**
     * @brief run will connect to the host given in the constructor; it won't do anything if
     * servCert was a NULL certificate. A qWarning() is emitted in that case.
     */
    void run();
};

#endif // CONTROLPLANECLIENT_H
