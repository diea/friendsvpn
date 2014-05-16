#ifndef DATAPLANECLIENT_H
#define DATAPLANECLIENT_H

#include <QObject>
#include "dataplaneconfig.h"
#include "bonjour/bonjourrecord.h"
#include "bonjoursql.h"
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>

class DataPlaneConnection;

class DataPlaneClient : public QObject
{
    Q_OBJECT
private:
    union {
        struct sockaddr_storage ss;
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
    } remote_addr, local_addr;

    char buf[BUFFER_SIZE];
    char addrbuf[INET6_ADDRSTRLEN];
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;
    int reading;
    struct timeval timeout;

    int fd;

    QHostAddress ip;

    DataPlaneConnection* con;

    QSocketNotifier* notif;

public:
    /**
     * @brief DataPlaneClient
     * @param rec: associated bonjour record for this data. This record should be resolved!
     *             ONLY First IP in Qlist will be used!
     * @param parent
     */
    ~DataPlaneClient();
    explicit DataPlaneClient(QHostAddress ip, DataPlaneConnection* con, QObject *parent = 0);

    void sendBytes(const char* bytes, socklen_t len);

    void stop();
signals:
    void bufferReady(const char* buf, int len);
private slots:
    void readyRead(int fd);
public slots:
    void run();
};

#endif // DATAPLANECLIENT_H
