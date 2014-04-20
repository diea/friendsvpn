#include "dataplaneconnection.h"

DataPlaneConnection::DataPlaneConnection(QObject *parent) :
    QObject(parent)
{
}

void DataPlaneConnection::start() {
    union {
        struct sockaddr_storage ss;
        struct sockaddr_in6 s6;
        struct sockaddr_in s4;
    } server_addr, client_addr;

    server_addr.s6.sin6_family = AF_INET6;
    server_addr.s6.sin6_addr = in6addr_any;
    server_addr.s6.sin6_port = htons(port);

    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;
    struct timeval timeout;
    const int on = 1, off = 0;

    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(DTLSv1_server_method());
    /* We accept all ciphers, including NULL.
     * Not recommended beyond testing and debugging
     */
    //SSL_CTX_set_cipher_list(ctx, "ALL:NULL:eNULL:aNULL");
    SSL_CTX_set_cipher_list(ctx, "DEFAULT");
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
    if (!SSL_CTX_use_certificate_file(ctx, "../servCert.pem", SSL_FILETYPE_PEM)) {
        qWarning << "ERROR: no certificate found!";
        exit(-1);
    }

    if (!SSL_CTX_use_PrivateKey_file(ctx, "../servKey.pem", SSL_FILETYPE_PEM)) {
        qWarning << "ERROR: no private key found!";
        exit(-1);
    }
    if (!SSL_CTX_check_private_key (ctx)) {
        qWarning << "ERROR: invalid private key!";
        exit(-1);
    }
}
