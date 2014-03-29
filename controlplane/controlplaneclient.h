#ifndef CONTROLPLANECLIENT_H
#define CONTROLPLANECLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QHostAddress>

class ControlPlaneClient: public QObject
{
    Q_OBJECT
private:
    QSslSocket* sslClient;
public:
    ControlPlaneClient(QHostAddress addr, int port, QObject* parent = 0);
    ~ControlPlaneClient();

public slots:
    void run();
};

#endif // CONTROLPLANECLIENT_H
