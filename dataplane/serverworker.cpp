#include "serverworker.h"
#include "dataplaneconnection.h"

#include <QDebug>
#include <QThread>
ServerWorker::ServerWorker(addrUnion server_addr, addrUnion client_addr, SSL* ssl, DataPlaneConnection* con, QObject *parent) :
    server_addr(server_addr), client_addr(client_addr), ssl(ssl), con(con), QObject(parent)
{
}

void ServerWorker::connection_handle() {
    qDebug() << "handling connection in thread" << QThread::currentThreadId();
    ssize_t len;
    char buf[BUFFER_SIZE];
    char addrbuf[INET6_ADDRSTRLEN];

    int reading = 0, ret;
    const int on = 1, off = 0;
    struct timeval timeout;
    int num_timeouts = 0, max_timeouts = 5;
    //fprintf(stderr, "ss_family client %d and server %d\n", client_addr.ss.ss_family, server_addr.ss.ss_family);
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
                 //printf("read %d bytes\n", (int) len);
                 //printf("%s \n", buf);
                 // TODO call dataplaneconnection
                 con->readBuffer(buf);
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
}

void ServerWorker::disconnect() {
    SSL_shutdown(ssl);
    close(fd);
    SSL_free(ssl);
    ERR_remove_state(0);
    printf("done, connection closed.\n");
    fflush(stdout);
}

void ServerWorker::sendBytes(const char* buf, int len) {
    qDebug() << "Sending bytes in server worker!";
    if (len > 0) {
        qDebug() << "SSL_write!";
        len = SSL_write(ssl, buf, len);
        qDebug() << "SSL_write out!";
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
             break;
         case SSL_ERROR_SSL:
             printf("SSL write error: ");
             //printf("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
             break;
         default:
             printf("Unexpected error while writing!\n");
             break;
        }
    }
    qDebug() << "Got out !";
}
