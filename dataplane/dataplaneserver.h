#ifndef DataPlaneServer_H
#define DataPlaneServer_H

#include <QObject>
#include "bonjoursql.h"
#include "dataplaneconfig.h"
#include "serverworker.h"
#include "connectioninitiator.h"

class DataPlaneServer : public QObject
{
    Q_OBJECT
private:
    QList<QThread*> threads;
    BonjourSQL* qSql;

    static int cookie_initialized;
    static unsigned char* cookie_secret;
    static DataPlaneServer* instance;

    static int dtls_verify_callback (int ok, X509_STORE_CTX *ctx);
    static int verify_cookie(SSL *ssl, unsigned char *cookie, unsigned int cookie_len);
    static int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len);

    explicit DataPlaneServer(QObject *parent = 0);
public:
    static DataPlaneServer* getInstance(QObject *parent = 0);

public slots:
    void start();

};

#endif // DataPlaneServer_H
