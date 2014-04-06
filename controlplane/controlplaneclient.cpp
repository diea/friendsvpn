#include "controlplaneclient.h"
#include "connectioninitiator.h"
ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    this->con = con;
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
    sslClient->close();
    delete sslClient;
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
    qDebug() << "Sending a write !!";
    // TODO protocol starts here
    // Send HELLO packet
    QString hello("HELLO\r\nUid:" + init->getMyUid() + "\r\n");
    sslClient->write(hello.toUtf8().constData());
    sslClient->flush();
    //sslClient->write("TEST TEST TEST2");

}

void ControlPlaneClient::sslClientReadyRead() {
    qDebug() << "Ready read !";
    if (!sslClient->isAssociated()) { // not associated with a ControlPlaneConnection
        char buf[300];
        sslClient->readLine(buf, 300);
        qDebug() << "ConstChar buffer" << buf;
        QString bufStr(buf);
        qDebug() << "QString buf" << bufStr;
        qDebug() << "It hangs right here";
        if (bufStr.startsWith("HELLO")) {
            QString uidStr(sslClient->readLine());
            uidStr.chop(2); // drop \r\0
            qDebug() << uidStr.remove(0, 4);
            // drop the Uid: part with the .remove and get the CPConnection* correspoding to this UID
            qDebug() << "Going into init";
            ControlPlaneConnection* con =  init->getConnection(bufStr.remove(0, 4));
            sslClient->setControlPlaneConnection(con); // associate the sslSock with it
            con->addMode(Client_mode, sslClient);
            qDebug() << "ssl Sock associated";
            //sslClient->write("TEST TEST TEST");
            //sslClient->flush();
        }
    } else { // socket is associated with controlplaneconnection
        sslClient->getControlPlaneConnection()->readBuffer(sslClient->readAll().data());
    }
}

