#include "connectioninitiator.h"
#include "controlplane/controlplaneclient.h"
#include "controlplane/controlplaneserver.h"
#include "controlplane/controlplaneconnection.h"

#include "dataplane/dataplaneserver.h"
#include "dataplane/dataplaneclient.h"
#include "dataplane/dataplaneconnection.h"
#include "unixsignalhandler.h"

ConnectionInitiator* ConnectionInitiator::instance = NULL;

ConnectionInitiator::ConnectionInitiator(QObject *parent) :
    QObject(parent)
{
    this->qSql = BonjourSQL::getInstance();

    // generate self-signed certificate
    // this bit is inspired by http://stackoverflow.com/questions/256405/programmatically-create-x509-certificate-using-openssl
    EVP_PKEY* pkey;
    pkey = EVP_PKEY_new();

    RSA* rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
        qWarning() << "Unable to generate 2048-bit RSA key";
        UnixSignalHandler::termSignalHandler(0);
    }

    X509* x509 = X509_new();
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), -2000);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);

    X509_set_pubkey(x509, pkey);

    X509_NAME * name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC,
                               (unsigned char *)"BE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC,
                               (unsigned char *)"FriendsVPN", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               (unsigned char *)"facebookApp", -1, -1, 0);
    X509_set_issuer_name(x509, name);

    if (!X509_sign(x509, pkey, EVP_sha1())) {
        qWarning() << "Error signing certificate";
        UnixSignalHandler::termSignalHandler(0);
    }

    // get the PEM string for cert and key

    // cert
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(bio, x509);
    BUF_MEM *bptr;
    BIO_get_mem_ptr(bio, &bptr);
    int length = bptr->length;
    char certBuf[length + 1];
    BIO_read(bio, certBuf, length);
    certBuf[length] = '\0';
    BIO_free(bio);

    // key
    bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, pkey, NULL, NULL, 0, NULL, NULL);
    BIO_get_mem_ptr(bio, &bptr);
    length = bptr->length;

    char keyBuf[length + 1];
    BIO_read(bio, keyBuf, length);
    keyBuf[length] = '\0';
    BIO_free(bio);

    qDebug() << keyBuf;
    qDebug() << certBuf;

    key = QSslKey(keyBuf, QSsl::Rsa, QSsl::Pem);
    cert = QSslCertificate(certBuf, QSsl::Pem);
    qSql->pushCert(cert);
}

ConnectionInitiator* ConnectionInitiator::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (instance == NULL) {
        instance = new ConnectionInitiator();
    }
    mutex.unlock();
    return instance;
}

void ConnectionInitiator::run() {
    // start the server
    this->startServer();
    // start the clients
    this->startClients();
}

void ConnectionInitiator::startServer() {
    server = new ControlPlaneServer(cert, key, QHostAddress::AnyIPv6, CONTROLPLANELISTENPORT, this);
    server->start();

    dpServer = DataPlaneServer::getInstance();
    dpServer->moveToThread(&dpServerThread);
    connect(&dpServerThread, SIGNAL(started()), dpServer, SLOT(start()));
    dpServerThread.start();
    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    connect(u, SIGNAL(exiting()), &dpServerThread, SLOT(quit()));
}

void ConnectionInitiator::startClients() {
    QList< User* > friends = qSql->getFriends();

    foreach(User* frien_d, friends) {
        ControlPlaneClient* c = new ControlPlaneClient(*(frien_d->cert), key, QHostAddress(*(frien_d->ipv6)),
                                                       CONTROLPLANELISTENPORT, *(frien_d->uid), this);
        c->run();
        clients.append(c);

        QThread* dcThread = new QThread(); // dataplane is threaded
        qDebug() << QHostAddress(*(frien_d->ipv6));

        DataPlaneConnection* con = this->getDpConnection(QString(*(frien_d->uid)));
        DataPlaneClient* dc = new DataPlaneClient(QHostAddress(*(frien_d->ipv6)), con);
        connect(dcThread, SIGNAL(started()), dc, SLOT(run()));
        connect(dcThread, SIGNAL(finished()), dcThread, SLOT(deleteLater()));
        connect(dc, SIGNAL(bufferReady(const char*, int)), con, SLOT(readBuffer(const char*, int)));
        UnixSignalHandler* u = UnixSignalHandler::getInstance();
        connect(u, SIGNAL(exiting()), dcThread, SLOT(quit()));
        dc->moveToThread(dcThread);

        /* we start the thread when the control plane connection is connected! */
        ControlPlaneConnection* controlPlane = this->getConnection(QString(*(frien_d->uid)));
        connect(controlPlane, SIGNAL(connected()), dcThread, SLOT(start()));

        // TODO delete user?
    }
}

ControlPlaneConnection* ConnectionInitiator::getConnection(QString uid) {
    static QMutex mutex;
    mutex.lock();
    QListIterator< ControlPlaneConnection* > i(instance->connections);
    while (i.hasNext()) {
        ControlPlaneConnection* nxt = i.next();
        if (nxt->getUid() == uid) {
            mutex.unlock();
            return nxt;
        }
    }
    // doesn't exist, create a new one
    ControlPlaneConnection* newCon = new ControlPlaneConnection(uid);
    instance->connections.append(newCon);
    mutex.unlock();
    return newCon;
}

DataPlaneConnection* ConnectionInitiator::getDpConnection(QString uid) {
    static QMutex mutex;
    mutex.lock();
    QListIterator< DataPlaneConnection * > i(instance->dpConnections);
    while (i.hasNext()) {
        DataPlaneConnection* nxt = i.next();
        if (nxt->getUid() == uid) {
            mutex.unlock();
            return nxt;
        }
    }
    // doesn't exist, create a new one
    DataPlaneConnection* newCon = new DataPlaneConnection(uid);
    instance->dpConnections.append(newCon);
    mutex.unlock();
    return newCon;
}

void ConnectionInitiator::removeConnection(ControlPlaneConnection *con) {
    instance->connections.removeAll(con);
}
void ConnectionInitiator::removeConnection(DataPlaneConnection *con) {
    instance->dpConnections.removeAll(con);
}

QString ConnectionInitiator::getMyUid() {
    return instance->qSql->getLocalUid();
}

QSslKey ConnectionInitiator::getPrivateKey() {
    return key;
}

QSslCertificate ConnectionInitiator::getLocalCertificate() {
    return cert;
}













