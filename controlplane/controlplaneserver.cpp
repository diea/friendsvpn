#include "controlplaneserver.h"
#include "connectioninitiator.h"
#include "unixsignalhandler.h"
ControlPlaneServer::ControlPlaneServer(QSslCertificate servCert, QSslKey myKey,
                                       QHostAddress listenAdr, int listenPort, QObject *parent) :
    QObject(parent)
{
    this->listenAdr = listenAdr;
    this->listenPort = listenPort;

    tcpSrv = new QTcpServer(this);

    BonjourSQL* qSql = BonjourSQL::getInstance();
    SSL_library_init();
    SSL_METHOD* constmethod = SSLv3_client_method();
    SSL_METHOD *method = static_cast<SSL_METHOD*>(malloc(sizeof(SSL_METHOD)));
    memcpy(method, constmethod, sizeof(SSL_METHOD));

    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = SSLv3_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        UnixSignalHandler::termSignalHandler(0);
    }

    // get certificate and key from SQL & use them
    QSslCertificate cert = qSql->getLocalCert();
    QByteArray certBytesPEM = cert.toPem();
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

    QSslKey key = qSql->getMyKey();
    QByteArray keyBytesPEM = key.toPem();
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
    SSL* ssl = SSL_new(ctx);              /* get new SSL state with context */
    SSL_set_fd(ssl, socket->socketDescriptor());      /* set connection socket to SSL state */
    SslSocket* sslSock = new SslSocket(ssl);
    sslSockList.append(sslSock);
    connect(sslSock, SIGNAL(encrypted()), this, SLOT(sslSockReady()));
    connect(sslSock, SIGNAL(disconnected()), this, SLOT(sslDisconnected()));

    sslSock->startServerEncryption();
}

void ControlPlaneServer::sslSockReady() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    connect(sslSock, SIGNAL(readyRead()), this, SLOT(sslSockReadyRead()));
    // send HELLO packet
    QString hello("Uid:" + init->getMyUid() + "\r\n");
    sslSock->write("HELLO\r\n\r\n", 9);
    sslSock->write(hello.toLatin1().constData(), hello.size());
}

void ControlPlaneServer::sslSockError(const QList<QSslError>& errors) {
    //SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    qDebug() << "ssl error";
    qDebug() << errors;
}

void ControlPlaneServer::sslSockReadyRead() {
    SslSocket* sslSock = qobject_cast<SslSocket*>(sender());
    static QMutex mutexx;
    if (!sslSock->isAssociated()) {
        mutexx.lock();
        if (sslSock->isAssociated()) // if we acquired lock, retest if was not associated
            mutexx.unlock();
    }
    if (!sslSock->isAssociated()) { // not associated with a ControlPlaneConnection
        char buf[300];
        sslSock->read(buf, 9);
        QString bufStr(buf);
        if (bufStr.startsWith("HELLO")) {
            sslSock->read(buf, 300);
            QString uidStr(buf);
            uidStr.chop(2); // drop \r\0
            // TODO double check UID is friend
            // drop the Uid: part with the .remove and get the CPConnection* correspoding to this UID
            ControlPlaneConnection* con = init->getConnection(uidStr.remove(0, 4));
            con->addMode(Receiving, sslSock); // add server mode
            sslSock->setControlPlaneConnection(con); // associate the sslSock with it
            mutexx.unlock();
        }
    } else { // socket is associated with controlplaneconnection
        char buf[2048];
        int bytesRead = sslSock->read(buf, 2048);
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

