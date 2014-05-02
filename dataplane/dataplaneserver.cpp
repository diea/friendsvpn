#include "dataplaneserver.h"
#include <sys/types.h>
#include <sys/socket.h>

int DataPlaneServer::cookie_initialized = 0;
unsigned char* DataPlaneServer::cookie_secret = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * COOKIE_SECRET_LENGTH));

DataPlaneServer* DataPlaneServer::instance = NULL;

DataPlaneServer::DataPlaneServer(QObject *parent) :
    QObject(parent)
{
    qSql = BonjourSQL::getInstance();
}

DataPlaneServer* DataPlaneServer::getInstance(QObject* parent) {
    static QMutex mut;
    mut.lock();
    if (!instance)
        instance = new DataPlaneServer(parent);
    mut.unlock();
    return instance;
}

void DataPlaneServer::start() {
    server_addr.s6.sin6_family = AF_INET6;
    // we listen on public IP, which is the one stored in the DB.
    struct in6_addr servIp;
    inet_pton(AF_INET6, qSql->getLocalIP().toUtf8().data(), &servIp) ;
    server_addr.s6.sin6_addr = servIp; //in6addr_any;
    server_addr.s6.sin6_port = htons(DATAPLANEPORT);

    SSL_CTX *ctx;
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

    // get certificate and key from SQL & use them
#if 1
    QSslCertificate cert = qSql->getLocalCert();
    QByteArray certBytesPEM = cert.toPem();
    char* x509buffer = certBytesPEM.data();

    BIO *bi;
    bi = BIO_new_mem_buf(x509buffer, certBytesPEM.length());
    X509 *x;
    x = PEM_read_bio_X509(bi, NULL, NULL, NULL);

    if (!SSL_CTX_use_certificate(ctx,x)) {
        qWarning() << "ERROR: no certificate found!";
        exit(-1);
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
        exit(-1);
    }

    if (pkey != NULL) EVP_PKEY_free(pkey);
    if (bi != NULL) BIO_free(bi);

#elif 1
    /* The way to go with Files */
    qDebug() << QString(QCoreApplication::applicationDirPath() + "/testCerts/server-key.pem").toUtf8().data();

    if (!SSL_CTX_use_certificate_file(ctx, QString(QCoreApplication::applicationDirPath() + "/testCerts/server-cert.pem").toUtf8().data(), SSL_FILETYPE_PEM)) {
        qWarning() << "ERROR: no certificate found!";
        exit(-1);
    }
    if (!SSL_CTX_use_PrivateKey_file(ctx, QString(QCoreApplication::applicationDirPath() + "/testCerts/server-key.pem").toUtf8().data(), SSL_FILETYPE_PEM)) {
        qWarning() << "ERROR: no private key found!";
        exit(-1);
    }
#endif

    if (!SSL_CTX_check_private_key (ctx)) {
        qWarning() << "ERROR: invalid private key!";
        exit(-1);
    }
    /* Client has to authenticate */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, dtls_verify_callback);

    SSL_CTX_set_read_ahead(ctx, 1);
    SSL_CTX_set_cookie_generate_cb(ctx, generate_cookie);
    SSL_CTX_set_cookie_verify_cb(ctx, verify_cookie);

    int fd = socket(server_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(-1);
    }

#ifdef WIN32
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, (socklen_t) sizeof(on));
#else
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &on, (socklen_t) sizeof(on));
#ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const void*) &on, (socklen_t) sizeof(on));
#endif
#endif

    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
    bind(fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in6));

    while (1) {
        memset(&client_addr, 0, sizeof(struct sockaddr_storage));

        /* Create BIO */
        bio = BIO_new_dgram(fd, BIO_NOCLOSE);

        /* Set and activate timeouts */
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

        ssl = SSL_new(ctx);

        SSL_set_bio(ssl, bio, bio);
        SSL_set_options(ssl, SSL_OP_COOKIE_EXCHANGE);

        while (DTLSv1_listen(ssl, &client_addr) <= 0);

        /*if (pthread_create( &tid, NULL, connection_handle, info) != 0) {
            perror("pthread_create");
            exit(-1);
        }*/
        connection_handle(); // we don't create a thread, maybe use Qt Thread here :)
    }
}

void* DataPlaneServer::connection_handle() {
    ssize_t len;
    char buf[BUFFER_SIZE];
    char addrbuf[INET6_ADDRSTRLEN];
    int fd, reading = 0, ret;
    const int on = 1, off = 0;
    struct timeval timeout;
    int num_timeouts = 0, max_timeouts = 5;
    fprintf(stderr, "ss_family client %d and server %d\n", client_addr.ss.ss_family, server_addr.ss.ss_family);
    OPENSSL_assert(client_addr.ss.ss_family == server_addr.ss.ss_family);
    fd = socket(client_addr.ss.ss_family, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*) &on, (socklen_t) sizeof(on));
    #ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const void*) &on, (socklen_t) sizeof(on));
    #endif

    // works only with ipv6
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
    bind(fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in6));
    ::connect(fd, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in6));

    /* Set new fd and set BIO to connected */
    BIO_set_fd(SSL_get_rbio(ssl), fd, BIO_NOCLOSE);
    BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_CONNECTED, 0, &client_addr.ss);

    /* Finish handshake */
    do { ret = SSL_accept(ssl); } while (ret == 0);

    if (ret < 0) {
        perror("SSL_accept");
        printf("%s\n", ERR_error_string(ERR_get_error(), buf));
    }

    /* Set and activate timeouts */
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    qDebug() << "Accepted connection";
    printf ("------------------------------------------------------------\n");
    X509_NAME_print_ex_fp(stdout, X509_get_subject_name(SSL_get_peer_certificate(ssl)),
                          1, XN_FLAG_MULTILINE);
    printf("\n\n Cipher: %s", SSL_CIPHER_get_name(SSL_get_current_cipher(ssl)));
    printf ("\n------------------------------------------------------------\n\n");

    while (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) && num_timeouts < max_timeouts) {
        reading = 1;
        while (reading) {
            len = SSL_read(ssl, buf, sizeof(buf));

            switch (SSL_get_error(ssl, len)) {
                case SSL_ERROR_NONE:
                 printf("read %d bytes\n", (int) len);
                 printf("%s \n", buf);
                 reading = 0;
                 break;
                case SSL_ERROR_WANT_READ:
                 /* Handle socket timeouts */
                 if (BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP, 0, NULL)) {
                     num_timeouts++;
                     reading = 0;
                 }
                 /* Just try again */
                 break;
                case SSL_ERROR_ZERO_RETURN:
                 reading = 0;
                 break;
                case SSL_ERROR_SYSCALL:
                 printf("Socket read error: ");
                 //if (!handle_socket_error()) goto cleanup;
                 reading = 0;
                 exit(-1);
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
        if (len > 0) {
            len = SSL_write(ssl, buf, len);
            switch (SSL_get_error(ssl, len)) {
             case SSL_ERROR_NONE:
                 printf("wrote %d bytes\n", (int) len);
                 break;
             case SSL_ERROR_WANT_WRITE:
                 /* Can't write because of a renegotiation, so
                  * we actually have to retry sending this message...
                  */
                 break;
             case SSL_ERROR_WANT_READ:
                 /* continue with reading */
                 break;
             case SSL_ERROR_SYSCALL:
                 printf("Socket write error: ");
                 //if (!handle_socket_error()) goto cleanup;
                 reading = 0;
                 break;
             case SSL_ERROR_SSL:
                 printf("SSL write error: ");
                 printf("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
                 break;
             default:
                 printf("Unexpected error while writing!\n");
                 break;
            }
        }
    }

    SSL_shutdown(ssl);

    close(fd);
    //free(info);
    SSL_free(ssl);
    ERR_remove_state(0);
    printf("done, connection closed.\n");
    fflush(stdout);
}

int DataPlaneServer::dtls_verify_callback(int ok, X509_STORE_CTX *ctx) {
    /* This function should ask the user
     * if he trusts the received certificate.
     * Here we always trust.
     */
    return 1;
}

int DataPlaneServer::verify_cookie(SSL *ssl, unsigned char *cookie, unsigned int cookie_len) {
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
        unsigned int length = 0, resultlength;
        union {
            struct sockaddr_storage ss;
            struct sockaddr_in6 s6;
            struct sockaddr_in s4;
        } peer;

        /* If secret isn't initialized yet, the cookie can't be valid */
        if (!cookie_initialized)
            return 0;

        /* Read peer information */
        (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

        /* Create buffer with peer's address and port */
        length = 0;
        switch (peer.ss.ss_family) {
            case AF_INET:
                length += sizeof(struct in_addr);
                break;
            case AF_INET6:
                length += sizeof(struct in6_addr);
                break;
            default:
                OPENSSL_assert(0);
                break;
        }
        length += sizeof(in_port_t);
        buffer = (unsigned char*) OPENSSL_malloc(length);

        if (buffer == NULL)
            {
            printf("out of memory\n");
            return 0;
            }

        switch (peer.ss.ss_family) {
            case AF_INET:
                memcpy(buffer,
                       &peer.s4.sin_port,
                       sizeof(in_port_t));
                memcpy(buffer + sizeof(in_port_t),
                       &peer.s4.sin_addr,
                       sizeof(struct in_addr));
                break;
            case AF_INET6:
                memcpy(buffer,
                       &peer.s6.sin6_port,
                       sizeof(in_port_t));
                memcpy(buffer + sizeof(in_port_t),
                       &peer.s6.sin6_addr,
                       sizeof(struct in6_addr));
                break;
            default:
                OPENSSL_assert(0);
                break;
        }

        /* Calculate HMAC of buffer using the secret */
        HMAC(EVP_sha1(), (const void*) cookie_secret, COOKIE_SECRET_LENGTH,
             (const unsigned char*) buffer, length, result, &resultlength);
        OPENSSL_free(buffer);

        if (cookie_len == resultlength && memcmp(result, cookie, resultlength) == 0)
            return 1;

        return 0;
}

int DataPlaneServer::generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len) {
    unsigned char *buffer, result[EVP_MAX_MD_SIZE];
        unsigned int length = 0, resultlength;
        union {
            struct sockaddr_storage ss;
            struct sockaddr_in6 s6;
            struct sockaddr_in s4;
        } peer;

        /* Initialize a random secret */
        if (!cookie_initialized)
            {
            if (!RAND_bytes(cookie_secret, COOKIE_SECRET_LENGTH))
                {
                printf("error setting random cookie secret\n");
                return 0;
                }
            cookie_initialized = 1;
            }

        /* Read peer information */
        (void) BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

        /* Create buffer with peer's address and port */
        length = 0;
        switch (peer.ss.ss_family) {
            case AF_INET:
                length += sizeof(struct in_addr);
                break;
            case AF_INET6:
                length += sizeof(struct in6_addr);
                break;
            default:
                OPENSSL_assert(0);
                break;
        }
        length += sizeof(in_port_t);
        buffer = (unsigned char*) OPENSSL_malloc(length);

        if (buffer == NULL)
            {
            printf("out of memory\n");
            return 0;
            }

        switch (peer.ss.ss_family) {
            case AF_INET:
                memcpy(buffer,
                       &peer.s4.sin_port,
                       sizeof(in_port_t));
                memcpy(buffer + sizeof(peer.s4.sin_port),
                       &peer.s4.sin_addr,
                       sizeof(struct in_addr));
                break;
            case AF_INET6:
                memcpy(buffer,
                       &peer.s6.sin6_port,
                       sizeof(in_port_t));
                memcpy(buffer + sizeof(in_port_t),
                       &peer.s6.sin6_addr,
                       sizeof(struct in6_addr));
                break;
            default:
                OPENSSL_assert(0);
                break;
        }

        /* Calculate HMAC of buffer using the secret */
        HMAC(EVP_sha1(), (const void*) cookie_secret, COOKIE_SECRET_LENGTH,
             (const unsigned char*) buffer, length, result, &resultlength);
        OPENSSL_free(buffer);

        memcpy(cookie, result, resultlength);
        *cookie_len = resultlength;

        return 1;
}
