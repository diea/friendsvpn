#include "sslsocket.h"
#include "config.h"
#include <QSslCipher>

/* reverse of strnstr from
 * http://stackoverflow.com/questions/20213799/finding-last-occurence-of-string
 */
char * strrstr(char *string, char *find, size_t len)
{
  char *cp;
  for (cp = string + len - 4; cp >= string; cp--)
  {
    if (strncmp(cp, find, 4) == 0)
        return cp;
  }
  return NULL;
}

SslSocket::~SslSocket() {
    SSL_free(ssl);         /* release SSL state */
    ::close(sd);          /* close connection */
}

SslSocket::SslSocket(SSL* ssl, QObject *parent) :
    QObject(parent), ssl(ssl)
{
    con = NULL;
    bytesRead = 0;
    nextFullPacket = 0;
}

void SslSocket::setControlPlaneConnection(ControlPlaneConnection *con) {
    this->con = con;
}

ControlPlaneConnection* SslSocket::getControlPlaneConnection() {
    return this->con;
}

bool SslSocket::isAssociated() {
    return (con != NULL);
}

void SslSocket::startServerEncryption() {
    int accRet = SSL_accept(ssl);
    if (accRet != 1) {     /* do SSL-protocol accept */
        qDebug() << "start server encryption error" << accRet;
        qDebug() << "error code " << SSL_get_error(ssl, accRet);
        ERR_print_errors_fp(stdout);
        fflush(stdout);
    }

    sd = SSL_get_fd(ssl);       /* get socket connection */
    notif = new QSocketNotifier(sd, QSocketNotifier::Read);
    connect(notif, SIGNAL(activated(int)), this, SLOT(getNewBytes()));
    emit encrypted();
}

void SslSocket::startClientEncryption() {
    int accRet = SSL_connect(ssl);
    if (accRet != 1) {    /* do SSL-protocol accept */
        qDebug() << "start client encryption error";
        qDebug() << "start server encryption error" << accRet;
        qDebug() << "error code " << SSL_get_error(ssl, accRet);

        ERR_print_errors_fp(stdout);
        fflush(stdout);
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    notif = new QSocketNotifier(sd, QSocketNotifier::Read);
    connect(notif, SIGNAL(activated(int)), this, SLOT(getNewBytes()));
    emit encrypted();
}

void SslSocket::write(const char *buf, int size) {
    if (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
        SSL_write(ssl, buf, size);
    } else {
        emit disconnected();
    }
}

void SslSocket::getNewBytes() {
    if (!(SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN)) {
        mutex.lock();
        int ret = SSL_read(ssl, buf + bytesRead, SSL_BUFFERSIZE - bytesRead);
        qDebug() << "ssl sock read " << ret << "bytes";
        qDebug() << "ssl sock read " << buf;
        qDebug() << "reading error code " << SSL_get_error(ssl, ret);
        if (ret > 0) {
            bytesRead += ret;
            if ((buf[bytesRead - 1] == '\n') && (buf[bytesRead - 2] == '\r') && (buf[bytesRead - 3] == '\n')
                && (buf[bytesRead - 4] == '\r')) {
                nextFullPacket = bytesRead;
            } else {
                char* packetEnd = strrstr(buf, "\r\n\r\n", bytesRead) + 4;
                int packetEndIndex = packetEnd - buf;
                if (packetEndIndex > 4) {
                    // one full packet
                    nextFullPacket = packetEndIndex;
                }
            }
        }
        qDebug() << "gonna unlock mutex in getnewbytes";
        mutex.unlock();
        if (nextFullPacket > 0) {
            emit readyRead();
        }
    } else {
        emit disconnected();
    }
}

int SslSocket::read(char* caller_buf) {
    qDebug() << "locking ssl socket mutex in read";
    mutex.lock();
    qDebug() << "got in";
    memcpy(caller_buf, buf, nextFullPacket);
    int ret = nextFullPacket;
    caller_buf[nextFullPacket] = '\0';
    bytesRead -= nextFullPacket;
    if (bytesRead > 0) {
        qDebug() << "we have" << bytesRead << "bytes left";
        memmove(buf, buf + nextFullPacket, bytesRead); // move everything back at start of buffer
    }
    mutex.unlock();
    return ret;
}

void SslSocket::close() {
    SSL_shutdown(ssl);
}
