#include "rawsocket.h"

RawSocket::RawSocket(int port)
{
    qDebug("yeay");
    sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        qDebug() << "Error opening socket";
    qDebug() << sockfd;

    struct sockaddr_in6 serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;

    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        qDebug("ERROR on binding");

    /*sockn = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
    connect(sockn, SIGNAL(activated(int)), this, SLOT(readPacket()));*/
    char buffer[256];
    struct sockaddr_in6 cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    qDebug() << "Im here";
    if (recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &cli_addr, &clilen)==-1)
        perror("RecvFrom");
    //printf("Received packet from %s:%d\nData: %s\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);
    qDebug() << "Closing" << buffer;
    close(sockfd);
}

void RawSocket::readPacket() {
    qDebug("test read");
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    char buffer[256];
    bzero(buffer, 256);

    int clisockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (clisockfd < 0)
        qDebug() << "Error on accept";

    int n = read(clisockfd, buffer, 255);
    if (n < 0)
        qDebug() << "Error reading from socket";
    qDebug() << "Message" << buffer;
    close(clisockfd);
}
