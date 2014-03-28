#ifndef RAWSOCKET_H
#define RAWSOCKET_H
#include <QObject>
#include <QSocketNotifier>
#include <QDebug>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>


/**
 * @brief The RawSocket class implements the C raw socket with functions that we need.
 */
class RawSocket : public QObject
{
    Q_OBJECT
private:
    int sockfd;
    QSocketNotifier* sockn;
public:
    RawSocket(int port);
private slots:
    void readPacket();
};

#endif // RAWSOCKET_H
