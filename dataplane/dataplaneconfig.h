#ifndef DATAPLANECONFIG_H
#define DATAPLANECONFIG_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#define BUFFER_SIZE 1550
#define COOKIE_SECRET_LENGTH 16

typedef union {
    struct sockaddr_storage ss;
    struct sockaddr_in6 s6;
} addrUnion;

#endif // DATAPLANECONFIG_H
