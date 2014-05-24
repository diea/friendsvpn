#include "controlplaneserver.h"
#include "connectioninitiator.h"
#include "unixsignalhandler.h"
#include <unistd.h>
#include <fcntl.h>

ControlPlaneServer::ControlPlaneServer(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    this->listenAdr = listenAdr;
    this->listenPort = listenPort;

    tcpSrv = new QTcpServer(this);

    BonjourSQL* qSql = BonjourSQL::getInstance();

    /* OpenSSL init code */
    SSL_library_init();

#ifdef __APPLE__
    SSL_METHOD* method;
#elif __GNUC__
    const SSL_METHOD* method;
#endif

    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = SSLv3_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        UnixSignalHandler::termSignalHandler(0);
    }

    // get certificate and key from SQL & use them
    QByteArray certBytesPEM = servCert.toPem();
    char* x509buffer = certBytesPEM.data();

    BIO *bi;
    bi = BIO_new_mem_buf(x509buffer, certBytesPEM.length());
    X509 *x;
    x = PEM_read_bio_X509(bi, NULL, NULL, NULL);

    if (!SSL_CTX_use_certificate(ctx,x)) {
        qWarning() << "ERROR: no certificate found!";
        UnixSignalHandler::termSignalHandler(0);
    }

    if (x != NULL) X509_free(x);
    if (bi != NULL) BIO_free(bi);

    QByteArray keyBytesPEM = myKey.toPem();
    char* keyBuffer = keyBytesPEM.data();

    bi = BIO_new_mem_buf(keyBuffer, keyBytesPEM.length());
    EVP_PKEY *pkey;
    pkey = PEM_read_bio_PrivateKey(bi, NULL, NULL, NULL);

    if (!SSL_CTX_use_PrivateKey(ctx, pkey)) {
        qWarning() << "ERROR: no private key found!";
        UnixSignalHandler::termSignalHandler(0);
    }

    if (pkey != NULL) EVP_PKEY_free(pkey);
    if (bi != NULL) BIO_free(bi);

    if (!SSL_CTX_check_private_key (ctx)) {
        qWarning() << "ERROR: invalid private key!";
        UnixSignalHandler::termSignalHandler(0);
    }
    /* end OpenSSL init */
}

ControlPlaneServer::~ControlPlaneServer()
{
    qDebug() << "Destroy control plane server";
    foreach (SslSocket* sock, sslSockList) {
        sock->close();
        delete sock;
    }

    tcpSrv->close();
    delete tcpSrv;
}

void ControlPlaneServer::start() {
    tcpSrv->listen(listenAdr, listenPort);
    connect(tcpSrv, SIGNAL(newConnection()), this, SLOT(newIncoming()));
}

void ControlPlaneServer::newIncoming() {
    qDebug() << "New incoming control Plane !";
    QTcpSocket* socket = tcpSrv->nextPendingConnection();
    qDebug() << "TCP socket is in" << socket->state() << "mode";

    SSL* ssl = SSL_new(ctx);              /* get new SSL state with context */
    int sd = socket->socketDescriptor();

    /* set socket to blocking mode */
    int flags = fcntl(sd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(sd, F_SETFL, flags);

    qDebug() << "socket descriptor is " << sd;
    SSL_set_fd(ssl, sd);      /* set connection socket to SSL state */
    SslSocket* sslSock = new SslSocket(ssl);
    sslSockList.append(sslSock);
    connect(sslSock, SIGNAL(encrypted()), this, SLOT(sslSockReady()));
    connect(sslSock, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));

    sslSock->startServerEncryption();
}

void ControlPlaneServer::sslSockReady() {
    qDebug() << "ssl sock ready!";
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    connect(sslSock, SIGNAL(readyRead()), this, SLOT(sslSockReadyRead()));
    // send HELLO packet
    QString hello("HELLO\r\nUid:" + init->getMyUid() + "\r\n\r\n");
    qDebug() << "ssl sock write";
    sslSock->write(hello.toLatin1().constData(), hello.size());
}

void ControlPlaneServer::sslSockError(const QList<QSslError>& errors) {
    //SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}

void ControlPlaneServer::sslSockReadyRead() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    qDebug() << "sslSockReady read";
    static QMutex mutexx;
    if (!sslSock->isAssociated()) {
        mutexx.lock();
        if (sslSock->isAssociated()) // if we acquired lock, retest if was not associated
            mutexx.unlock();
    }
    if (!sslSock->isAssociated()) { // not associated with a ControlPlaneConnection
        char buf[SSL_BUFFERSIZE];
        qDebug() << "going into read";
        sslSock->read(buf);
        qDebug() << "gout out of read";
        QString bufStr(buf);
        QStringList packet = bufStr.split("\r\n", QString::SkipEmptyParts);
        qDebug() << "packet " << packet;
        if (packet.at(0).startsWith("HELLO")) {
            QStringList val = packet.at(1).split(":");
            // TODO double check UID is friend
            qDebug() << "get connection for " << val.at(1);

            ControlPlaneConnection* con = init->getConnection(val.at(1));
            con->addMode(Receiving, sslSock); // add server mode
            sslSock->setControlPlaneConnection(con); // associate the sslSock with it
            mutexx.unlock();
        }
    } else { // socket is associated with controlplaneconnection
        char buf[SSL_BUFFERSIZE];
        int bytesRead = sslSock->read(buf);
        sslSock->getControlPlaneConnection()->readBuffer(buf, bytesRead);
    }
}

void ControlPlaneServer::sslDisconnected() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    sslSockList.removeAll(sslSock);
    if (sslSock->isAssociated())
        sslSock->getControlPlaneConnection()->removeMode(Receiving);
    sslSock->deleteLater();
}

