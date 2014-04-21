#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "bonjoursql.h"

class DataPlaneConnection : public QObject
{
    Q_OBJECT
private:
    const int BUFFER_SIZE; // = (1<<16);
    BonjourSQL* qSql;
    SSL *ssl;
    union {
        struct sockaddr_storage ss;
        struct sockaddr_in6 s6;
        struct sockaddr_in s4;
    } server_addr, client_addr;

    static const int COOKIE_SECRET_LENGTH;// = 16;
    static int cookie_initialized;// = 0;
    static unsigned char* cookie_secret; //[COOKIE_SECRET_LENGTH];

    static int dtls_verify_callback (int ok, X509_STORE_CTX *ctx);
    static int verify_cookie(SSL *ssl, unsigned char *cookie, unsigned int cookie_len);
    static int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len);
    void* connection_handle();

public:
    explicit DataPlaneConnection(QObject *parent = 0);

    void start();
signals:

public slots:

};

#endif // DATAPLANECONNECTION_H
