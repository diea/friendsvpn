#include "controlplaneclient.h"
#include "connectioninitiator.h"
ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    this->addr = addr;
    this->port = port;
    this->servCert = servCert;

    sslClient = new SslSocket();

    sslClient->addCaCertificate(servCert);
    sslClient->setPrivateKey(myKey);
    sslClient->setPeerVerifyName("facebookApp"); // common name for all our certificates

    init = ConnectionInitiator::getInstance();
}

ControlPlaneClient::~ControlPlaneClient()
{
    qDebug() << "Destroy contorl plane client";
    sslClient->close();
    //delete sslClient;
}

void ControlPlaneClient::run() {
    if (servCert.isNull()) {
        qWarning() << "Warning, host " << this->addr << " has not set a certificate for SSL dataplane "
                    "connection";
        return; // do nothing if serv cert was not set!
    }

    // ignore errors since we are using self-signed certificates
    // connect(sslClient, SIGNAL(sslErrors(const QList<QSslError>&)), sslClient, SLOT(ignoreSslErrors()));
    connect(sslClient, SIGNAL(sslErrors(const QList<QSslError>&)), this,
            SLOT(sslErrors(const QList<QSslError>&)));
    connect(sslClient, SIGNAL(encrypted()), this, SLOT(connectionReady()));
    connect(sslClient, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));

    qDebug() << "CONNECT TO HOST " << addr.toString() << ":" << port;
    sslClient->connectToHostEncrypted(addr.toString(), port);
}

void ControlPlaneClient::sslErrors(const QList<QSslError>& errors) {
    //QSslSocket* sslSock = qobject_cast<QSslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}

void ControlPlaneClient::connectionReady() {
    connect(sslClient, SIGNAL(readyRead()), this, SLOT(sslClientReadyRead()));
    // TODO protocol starts here
    // Send HELLO packet
    QString hello("HELLO\r\nUid:" + init->getMyUid() + "\r\n");
    sslClient->write(hello.toUtf8().constData());
    sslClient->flush();
}

void ControlPlaneClient::sslClientReadyRead() {
    static QMutex mutexx;
    if (!sslClient->isAssociated()) {
        mutexx.lock();
        if (sslClient->isAssociated())
            mutexx.unlock();
    }
    if (!sslClient->isAssociated()) { // not associated with a ControlPlaneConnection
        char buf[300];
        sslClient->readLine(buf, 300);
        QString bufStr(buf);
        if (bufStr.startsWith("HELLO")) {
            sslClient->readLine(buf, 300);
            QString uidStr(buf);
            uidStr.chop(2); // drop \r\0
            //qDebug() << uidStr.remove(0, 4);
            // drop the Uid: part with the .remove and get the CPConnection* correspoding to this UID
            ControlPlaneConnection* con =  init->getConnection(uidStr.remove(0, 4));
            con->addMode(Emitting, sslClient); // add server mode
            sslClient->setControlPlaneConnection(con); // associate the sslSock with it
            mutexx.unlock();
        }
    } else { // socket is associated with controlplaneconnection
        //qDebug() << sslClient->readAll();
        QByteArray bytesBuf = sslClient->readAll();
        sslClient->getControlPlaneConnection()->readBuffer(bytesBuf.data(), bytesBuf.length());
    }
}

void ControlPlaneClient::sslDisconnected() {
    qDebug() << "Disco";
    if (sslClient->isAssociated())
        sslClient->getControlPlaneConnection()->removeMode(Emitting);
    sslClient->deleteLater();
}

