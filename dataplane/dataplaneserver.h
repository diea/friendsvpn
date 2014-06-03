#ifndef DataPlaneServer_H
#define DataPlaneServer_H

#include <QObject>
#include "databasehandler.h"
#include "dataplaneconfig.h"
#include "serverworker.h"
#include "connectioninitiator.h"
/**
 * @brief The DataPlaneServer class waits and accepts (if connection is from a friend)
 * incoming data plane connections.
 */
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
    /**
     * @brief readyRead a new incoming connection, creates a ServerWorker to accept this new
     * connection
     * @param fd
     */
    void readyRead(int fd);
public slots:
    void start();

};

#endif // DataPlaneServer_H
