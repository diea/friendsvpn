#ifndef DATAPLANECONNECTION_H
#define DATAPLANECONNECTION_H

#include <QObject>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>

class DataPlaneConnection : public QObject
{
    Q_OBJECT
private:
    const int BUFFER_SIZE; // = (1<<16);
    const int COOKIE_SECRET_LENGTH;// = 16;
    int cookie_initialized;// = 0;
    unsigned char* cookie_secret; //[COOKIE_SECRET_LENGTH];

    int dtls_verify_callback (int ok, X509_STORE_CTX *ctx);
    int verify_cookie(SSL *ssl, unsigned char *cookie, unsigned int cookie_len);
    int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len);
    void* connection_handle(void *info);

public:
    explicit DataPlaneConnection(QObject *parent = 0);

    void start();
signals:

public slots:

};

#endif // DATAPLANECONNECTION_H
