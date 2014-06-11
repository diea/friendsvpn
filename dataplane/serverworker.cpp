#include "serverworker.h"
#include "dataplaneconnection.h"

#include <QDebug>
#include <QThread>
ServerWorker::ServerWorker(addrUnion server_addr, addrUnion client_addr, SSL* ssl, DataPlaneConnection* con, QObject *parent) :
    QObject(parent), server_addr(server_addr), client_addr(client_addr), ssl(ssl), con(con)
{
    notif = NULL;
}

void ServerWorker::connection_handle() {
    char buf[BUFFER_SIZE];

    int ret;
    const int on = 1, off = 0;

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
    struct timeval timeout;
    /* Set and activate timeouts */
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);

    qDebug() << "Accepted connection";
    qDebug("------------------------------------------------------------");
    /*X509_NAME_print_ex_fp(stdout, X509_get_subject_name(SSL_get_peer_certificate(ssl)),
                          1, XN_FLAG_MULTILINE);*/
    qDebug() << "\n\n Cipher: " << SSL_CIPHER_get_name(SSL_get_current_cipher(ssl));
    qDebug("\n------------------------------------------------------------");

    notif = new QSocketNotifier(fd, QSocketNotifier::Read);
    connect(notif, SIGNAL(activated(int)), this, SLOT(readyRead(int)));

    con->addMode(Receiving, this);
}

void ServerWorker::readyRead(int) {
    //notif->setEnabled(false);
    size_t len;
    if ((!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN))) {
        char* buf = static_cast<char*>(malloc(BUFFER_SIZE * sizeof(char)));
        memset(buf, 0, BUFFER_SIZE * sizeof(char));
        closeProtect.lock();
        if ((len = SSL_read(ssl, buf, BUFFER_SIZE * sizeof(char))) > 0) {
            switch (SSL_get_error(ssl, len)) {
                case SSL_ERROR_NONE:
                qDebug() << "Reading in thread" << QThread::currentThreadId();
                 con->readBuffer(buf, len);
                 break;
                case SSL_ERROR_WANT_READ:
                 /* Handle socket timeouts */
                 if (BIO_ctrl(SSL_get_rbio(ssl), BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP, 0, NULL)) { }
                 /* Just try again */
                 break;
                case SSL_ERROR_ZERO_RETURN:
                 break;
                case SSL_ERROR_SYSCALL:
                 qDebug("Socket read error: ");
                 qDebug() << ERR_error_string(ERR_get_error(), buf);
                 qDebug() << SSL_get_error(ssl, len);
                 break;
                case SSL_ERROR_SSL:
                 qDebug("SSL read error: ");
                 qDebug("%s (%d)\n", ERR_error_string(ERR_get_error(), buf), SSL_get_error(ssl, len));
                 break;
                default:
                 qDebug("Unexpected error while reading!\n");
                 break;
            }
        }
        closeProtect.unlock();
        free(buf);
    }
    //notif->setEnabled(true);
}

void ServerWorker::stop() {
    closeProtect.lock();
    notif->setEnabled(false);
    SSL_shutdown(ssl);
    close(fd);
    SSL_free(ssl);
    ERR_remove_state(0);
    printf("done, server worker connection closed.\n");
    fflush(stdout);
    this->deleteLater();
}

void ServerWorker::sendBytes(const char* buf, int len) {
    if (len > 0) {
        qDebug() << "Writing in thread" << QThread::currentThreadId();
        len = SSL_write(ssl, buf, len);
        switch (SSL_get_error(ssl, len)) {
         case SSL_ERROR_NONE:
             qDebug() << "SSL_ERROR_NONE";
             break;
         case SSL_ERROR_WANT_WRITE:
             /* Can't write because of a renegotiation, so
              * we actually have to retry sending this message...
              */
             qDebug() << "SSL_ERROR_WANT_WRITE";
             break;
         case SSL_ERROR_WANT_READ:
             qDebug() << "SSL_ERROR_WANT_READ";
             /* continue with reading */
             break;
         case SSL_ERROR_SYSCALL:
             qDebug("Socket write error: ");
             break;
         case SSL_ERROR_SSL: { /* {} to help compiler understand scope of bufCpy */
             qDebug("SSL write error: ");
             char bufCpy[len];
             memcpy(bufCpy, buf, len);
             qDebug() << ERR_error_string(ERR_get_error(), bufCpy) << "(" << SSL_get_error(ssl, len) << ")";
             break;
         }
         default:
             qDebug("Unexpected error while writing!");
             break;
        }
    }
}

ServerWorker::~ServerWorker()
{
    if (notif)
        delete notif;
}
