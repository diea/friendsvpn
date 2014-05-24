#include "controlplaneclient.h"
#include "connectioninitiator.h"

#include <unistd.h>
#include <fcntl.h>

ControlPlaneClient::ControlPlaneClient(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress addr, int port, QObject *parent) :
    QObject(parent)
{
    this->addr = addr;
    this->port = port;
    this->servCert = servCert;

    /* openSSL init code */
    SSL_library_init();
#ifdef __APPLE__
    SSL_METHOD* method = SSLv3_client_method();
#elif __GNUC__
    const SSL_METHOD* method = SSLv3_client_method();
#endif

    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = SSLv3_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* end openssl init */

    init = ConnectionInitiator::getInstance();
}

ControlPlaneClient::~ControlPlaneClient()
{
    qDebug() << "Destroy control plane client";
    sslClient->close();
    //delete sslClient;
}

void ControlPlaneClient::run() {
    if (servCert.isNull()) {
        qWarning() << "Warning, host " << this->addr << " has not set a certificate for SSL dataplane "
                    "connection";
        return; // do nothing if serv cert was not set!
    }
    connect(&sock, SIGNAL(connected()), this, SLOT(connectSSL()));
    sock.connectToHost(addr.toString(), port);
}

void ControlPlaneClient::connectSSL() {
    SSL* ssl = SSL_new(ctx);      /* create new SSL connection state */

    int sd = sock.socketDescriptor();
    /* set socket to blocking mode */
    int flags = fcntl(sd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(sd, F_SETFL, flags);

    SSL_set_fd(ssl, sd);    /* attach the socket descriptor */
    sslClient = new SslSocket(ssl);
    connect(sslClient, SIGNAL(encrypted()), this, SLOT(connectionReady()));
    connect(sslClient, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));
    sslClient->startClientEncryption();
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
    QString hello("HELLO\r\nUid:" + init->getMyUid() + "\r\n\r\n");
    sslClient->write(hello.toUtf8().constData(), hello.length());
}

void ControlPlaneClient::sslClientReadyRead() {
    static QMutex mutexx;
    if (!sslClient->isAssociated()) {
        mutexx.lock();
        if (sslClient->isAssociated())
            mutexx.unlock();
    }
    if (!sslClient->isAssociated()) { // not associated with a ControlPlaneConnection
        char buf[SSL_BUFFERSIZE];
        sslClient->read(buf);
        QString bufStr(buf);
        if (bufStr.startsWith("HELLO")) {
            sslClient->read(buf);
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
        char buf[SSL_BUFFERSIZE];
        int bytesRead = sslClient->read(buf);
        sslClient->getControlPlaneConnection()->readBuffer(buf, bytesRead);
    }
}

void ControlPlaneClient::sslDisconnected() {
    qDebug() << "Disco";
    if (sslClient->isAssociated())
        sslClient->getControlPlaneConnection()->removeMode(Emitting);
    sslClient->deleteLater();
}

