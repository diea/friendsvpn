#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

/**
 * usage: sockType ipProto port addr
 *
 * needs to be run as root on linux only
 *
 * return: 1 wrong arguments
 * return: 2 unable to create socket
 * return: 3 unable to bind
 */
int main(int argc, char** argv) {
    setuid(0);

    int fd = socket(AF_INET6, sockType, ipProto);
    if (fd < 0) {
        return 2;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = sockType;

    int portno = atoi(argv[1]);
    int server = getaddrinfo(listenIp.toUtf8().data(), NULL, &hints, &res);
    if (server) {
        qDebug("ERROR, no such host");
        UnixSignalHandler::termSignalHandler(0);
    }
        ((struct sockaddr_in6*) res->ai_addr)->sin6_port = htons(portno);

        qDebug() << "bind call";
        if (bind(fd, res->ai_addr, sizeof(struct sockaddr_in6)) < 0) {
            qDebug() << "error on bind" << errno;
            if (errno == EADDRNOTAVAIL) {
                // loop again until IP is available but just sleep a moment
                qDebug() << "Bind ERROR: EADDRNOTAVAIL";
                QThread::sleep(2);
            } else if (errno == EACCES) {
                if (port < 60000) {
                    port = 60001;
                } else {
                    port++;
                }
                if (port >= 65535) {
                    qWarning("Port escalation higher than 65535");
                    UnixSignalHandler::termSignalHandler(0);
                }
                qDebug() << "Could not bind on port " << listenPort << "going to use " << QString::number(port);
            }
        } else {
            bound = true;
        }
    }

#ifndef __APPLE__ /* linux */
    /* on linux the "bind" trick does not work due to bind's implementation needing a "listen"
     * to accept connections, we will thus prevent the kernel from sending its RST using an ip6tables
     * rule
     * we still bind on linux to prevent other applications from binding on the same port
     */

    char* ip6tablesRule = malloc(400 * sizeof(char));
    sprintf(ip6tablesRule, "ip6tables -A OUTPUT -s %s -p tcp --sport %s --tcp-flags RST RST -j DROP", argv[2], argv[1]);
    system(ip6tablesRule);
#endif

    return 0;
}
