#include "dataplaneclient.h"
#include "dataplaneconnection.h"

DataPlaneClient::DataPlaneClient(QHostAddress ip, DataPlaneConnection* con, QObject *parent) :
    QObject(parent), reading(0), ip(ip), con(con)
{
    memset((void *) &remote_addr, 0, sizeof(struct sockaddr_storage));
    memset((void *) &local_addr, 0, sizeof(struct sockaddr_storage));
    notif = NULL;
}

void DataPlaneClient::run() {
    inet_pton(AF_INET6, ip.toString().toUtf8().data(), &remote_addr.s6.sin6_addr);
    remote_addr.s6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
    remote_addr.s6.sin6_len = sizeof(struct sockaddr_in6);
#endif
    remote_addr.s6.sin6_port = htons(DATAPLANEPORT);

    fd = socket(remote_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        throw "Not able to create new socket";
    }
    BonjourSQL* qSql = BonjourSQL::getInstance();
    inet_pton(AF_INET6, qSql->getLocalIP().toUtf8().data(), &local_addr.s6.sin6_addr);
    local_addr.s6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
    local_addr.s6.sin6_len = sizeof(struct sockaddr_in6);
#endif
    local_addr.s6.sin6_port = htons(0);
    OPENSSL_assert(remote_addr.ss.ss_family == local_addr.ss.ss_family);
    bind(fd, (const struct sockaddr *) &local_addr, sizeof(struct sockaddr_in6));

    OpenSSL_add_ssl_algorithms();
    //SSL_load_error_strings();
    ctx = SSL_CTX_new(DTLSv1_client_method());

    if(!SSL_CTX_set_cipher_list(ctx, "eNULL:NULL")) {
        fprintf(stderr, "ssl_ctx fail\n");
        return;
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
        return;
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
        return;
    }

    if (pkey != NULL) EVP_PKEY_free(pkey);
    if (bi != NULL) BIO_free(bi);

    if (!SSL_CTX_check_private_key (ctx)) {
        qWarning() << "ERROR: invalid private key!";
        return;
    }

    SSL_CTX_set_verify_depth (ctx, 2);
    SSL_CTX_set_read_ahead(ctx, 1);

    ssl = SSL_new(ctx);

    /* Create BIO, connect and set to already connected */
    bio = BIO_new_dgram(fd, BIO_CLOSE);
    ::connect(fd, (struct sockaddr *) &remote_addr, sizeof(struct sockaddr_in6));
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &remote_addr.ss);

    SSL_set_bio(ssl, bio, bio);

    if (SSL_connect(ssl) < 0) {
        perror("SSL_connect");
        //printf("%s\n", ERR_error_string(ERR_get_error(), buf));
        return;
    }

    /* Set and activate timeouts */
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    printf ("\nConnected to %s\n",
             inet_ntop(AF_INET6, &remote_addr.s6.sin6_addr, addrbuf, INET6_ADDRSTRLEN));

    con->addMode(Emitting, this);

    if (SSL_get_peer_certificate(ssl)) {
        printf ("------------------------------------------------------------\n");
        X509_NAME_print_ex_fp(stdout, X509_get_subject_name(SSL_get_peer_certificate(ssl)),
                              1, XN_FLAG_MULTILINE);
        printf("\n\n Cipher: %s", SSL_CIPHER_get_name(SSL_get_current_cipher(ssl)));
        printf ("\n------------------------------------------------------------\n\n");
    }

    notif = new QSocketNotifier(fd, QSocketNotifier::Read);
    connect(notif, SIGNAL(activated(int)), this, SLOT(readyRead(int)));
}

void DataPlaneClient::readyRead(int fd) {
    size_t len;
    if (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
        len = SSL_read(ssl, buf, sizeof(buf));

        switch (SSL_get_error(ssl, len)) {
            case SSL_ERROR_NONE:
             emit bufferReady(buf, len);
             break;
            case SSL_ERROR_WANT_READ:
             /* Handle socket timeouts */
             if (BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP, 0, NULL)) {
                 //num_timeouts++;
             }
             /* Just try again */
             break;
            case SSL_ERROR_ZERO_RETURN:
             reading = 0;
             break;
            case SSL_ERROR_SYSCALL:
             printf("Socket read error");
             //if (!handle_socket_error()) goto cleanup;
             reading = 0;
             //exit(-1);
             break;
            case SSL_ERROR_SSL:
             printf("SSL read error: ");
             printf("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
             break;
            default:
             printf("Unexpected error while reading!\n");
             break;
        }
    }
}

void DataPlaneClient::sendBytes(const char *bytes, socklen_t len) {
    if (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
        SSL_write(ssl, bytes, len);
        switch (SSL_get_error(ssl, len)) {
            case SSL_ERROR_NONE:
                break;
            case SSL_ERROR_WANT_WRITE:
                /* Just try again later */
                break;
            case SSL_ERROR_WANT_READ:
                /* continue with reading */
                break;
            case SSL_ERROR_SYSCALL:
                printf("Socket write error: ");
                //if (!handle_socket_error()) exit(1);
                reading = 0;
                break;
            case SSL_ERROR_SSL:
                printf("SSL write error: ");
                //printf("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
                return;
                break;
            default:
                printf("Unexpected error while writing!\n");
                return;
                break;
        }
    }
}

void DataPlaneClient::stop() {
    SSL_shutdown(ssl);
    close(fd);
    SSL_free(ssl);
    ERR_remove_state(0);
    this->deleteLater();
}

DataPlaneClient::~DataPlaneClient() {
    if (notif)
        delete notif;
}
