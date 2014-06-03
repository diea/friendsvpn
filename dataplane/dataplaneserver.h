#ifndef DataPlaneServer_H
#define DataPlaneServer_H

#include <QObject>
#include "databasehandler.h"
#include "dataplaneconfig.h"
#include "serverworker.h"
#include "connectioninitiator.h"

class DataPlaneServer : public QObject
{
    Q_OBJECT
private:
    QList<QThread*> threads;
    DatabaseHandler* qSql;

    union {
        struct sockaddr_storage ss;
        struct sockaddr_in6 s6;
    } server_addr, client_addr;
    SSL_CTX *ctx;
    BIO *bio;
    SSL* ssl;
    struct timeval timeout;
    int fd;
    QSocketNotifier* notif;

    static int cookie_initialized;
    static unsigned char* cookie_secret;
    static DataPlaneServer* instance;

    static int dtls_verify_callback (int ok, X509_STORE_CTX *ctx);
    static int verify_cookie(SSL *ssl, unsigned char *cookie, unsigned int cookie_len);
    static int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len);

    explicit DataPlaneServer(QObject *parent = 0);
public:
    static DataPlaneServer* getInstance(QObject *parent = 0);
private slots:
    void readyRead(int fd);
public slots:
    void start();

};

#endif // DataPlaneServer_H
