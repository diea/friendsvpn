#include "rawsockets.h"
#include "string.h"
#include <QMutex>
#include <QFile>

#ifndef __APPLE__
#include <ifaddrs.h>
#include <linux/if_link.h>
#endif

RawSockets* RawSockets::instance = NULL;

RawSockets::RawSockets(QObject *parent) :
    QObject(parent)
{
#ifdef __APPLE__
    struct ifaddrs *ifap, *ifaptr;
    unsigned char *ptr;
    if (getifaddrs(&ifap) == 0) {
        for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
            if ((((ifaptr)->ifa_addr)->sa_family == AF_LINK) && (!strncmp((ifaptr)->ifa_name, "en", 2))) {
                struct rawProcess* r = static_cast<struct rawProcess*>(malloc(sizeof(struct rawProcess)));
                memset(r, 0, sizeof(struct rawProcess));
                r->linkType = DLT_EN10MB;
                ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)(ifaptr)->ifa_addr);
                memcpy(&r->mac, ptr, ETHER_ADDR_LEN);

                r->process = new QProcess(); // can always connect signals in each Proxy's constructor.
                QStringList arguments;
                arguments.append(ifaptr->ifa_name);
                r->process->start(QString(HELPERPATH) + "sendRaw", arguments);
                r->process->waitForStarted();
                rawHelpers.insert(ifaptr->ifa_name, r);
            }
        }
        freeifaddrs(ifap);
    }

    // set loopback
    struct rawProcess* r = static_cast<struct rawProcess*>(malloc(sizeof(struct rawProcess)));
    memset(r, 0, sizeof(struct rawProcess));
    r->linkType = DLT_NULL;

    r->process = new QProcess();
    QStringList arguments;
    arguments.append("lo0");
    r->process->start(QString(HELPERPATH) + "sendRaw", arguments);
    r->process->waitForStarted();
    rawHelpers.insert("lo0", r);
#elif __GNUC__ /* inspired by http://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program */
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) { /* handle error*/ };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        /* handle error */
        qFatal("ioctl error while getting interfaces");
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if ((ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) && (!strncmp(ifr.ifr_ifrn.ifrn_name, "eth", 3))) {
                    struct rawProcess* r = static_cast<struct rawProcess*>(malloc(sizeof(struct rawProcess)));
                    memset(r, 0, sizeof(struct rawProcess));
                    r->linkType = DLT_EN10MB;
                    memcpy(&r->mac, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

                    r->process = new QProcess(); // can always connect signals in each Proxy's constructor.
                    QStringList arguments;
                    arguments.append(ifr.ifr_ifrn.ifrn_name);
                    r->process->start(QString(HELPERPATH) + "sendRaw", arguments);
                    r->process->waitForStarted();
                    rawHelpers.insert(ifr.ifr_ifrn.ifrn_name, r);
                }
            }
        }
        else {
            /* handle error */
            qFatal("ioctl error while getting interfaces");
        }
    }

    // set loopback
    struct rawProcess* r = static_cast<struct rawProcess*>(malloc(sizeof(struct rawProcess)));
    memset(r, 0, sizeof(struct rawProcess));
    r->linkType = DLT_EN10MB; /* loopback is Ethernet on linux */
    r->process = new QProcess();
    QStringList arguments;
    arguments.append("lo");
    r->process->start(QString(HELPERPATH) + "sendRaw", arguments);
    r->process->waitForStarted();
    rawHelpers.insert("lo", r);
#endif
}

RawSockets* RawSockets::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (!instance) {
        instance =  new RawSockets();
    }
    mutex.unlock();
    return instance;
}

void RawSockets::writeBytes(QString srcIp, QString dstIp, int srcPort, const char *transAndPayload, int sockType, int packet_send_size) {
    IpResolver* r = IpResolver::getInstance();
    struct ip_mac_mapping map = r->getMapping(dstIp);

    if (map.interface == "") {
        qWarning() << "Mapping not found, cannot send packet!";
        return;
    }

    //write.lock(); // not optimal to lock here, but try to avoid mem corruption by using single buffer

    struct rawProcess* p = rawHelpers.value(map.interface);
    QProcess* raw = p->process;
    if (!raw || raw->state() != 2) {
        qFatal("No raw helper");
    }

    int linkLayerType = DLT_EN10MB;
#ifdef __APPLE__
    if (map.mac.isEmpty()) {
        linkLayerType = DLT_NULL;
    }
#endif

    int bufferSize = packet_send_size + sizeof(struct rawComHeader); /* rawComHeader contains
                                                                      * ethernet and IPv6 header */
    char buffer[bufferSize];

    /* copy trans and payload to buffer, after where the rawComHeader is going */
    memcpy(buffer + sizeof(struct rawComHeader), transAndPayload, packet_send_size);

    char* packet_send = buffer + sizeof(struct rawComHeader);

    struct rawComHeader rawHeader;
    memset(&rawHeader, 0, sizeof(struct rawComHeader));
    rawHeader.payload_len = packet_send_size;

    if (linkLayerType == DLT_EN10MB) {
        rawHeader.linkHeader.ethernet.ether_type = htons(ETH_IPV6);
        // set source MAC
        const unsigned char* source_mac_addr;
        const char* if_name = map.interface.toUtf8().data();
#ifdef __APPLE__
        struct ifaddrs *ifap, *ifaptr;
        unsigned char *ptr = NULL;
        if (getifaddrs(&ifap) == 0) {
            for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
                if (!strcmp((ifaptr)->ifa_name, if_name) && (((ifaptr)->ifa_addr)->sa_family == AF_LINK)) {
                    ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)(ifaptr)->ifa_addr);
                    break;
                }
            }
            freeifaddrs(ifap);
        }
        source_mac_addr = ptr;
#elif __GNUC__
        struct ifreq ifr;
        size_t if_name_len=strlen(if_name);
        if (if_name_len<sizeof(ifr.ifr_name)) {
            memcpy(ifr.ifr_name,if_name,if_name_len);
            ifr.ifr_name[if_name_len]=0;
        } else {
            fprintf(stderr,"interface name is too long");
            exit(1);
        }
        // Open an IPv4-family socket for use when calling ioctl.
        int fd=socket(AF_INET,SOCK_DGRAM,0);
        if (fd==-1) {
            perror(0);
            exit(1);
        }
        // Obtain the source MAC address, copy into Ethernet header
        if (ioctl(fd,SIOCGIFHWADDR,&ifr)==-1) {
            perror(0);
            close(fd);
            exit(1);
        }

        source_mac_addr = (unsigned char*)ifr.ifr_hwaddr.sa_data;
        close(fd);
#endif

        memcpy(rawHeader.linkHeader.ethernet.ether_shost, source_mac_addr, ETHER_ADDR_LEN);
        // set dst mac
        char* dstMac = map.mac.toUtf8().data();
        if (!map.mac.isEmpty()) {
            char* token;
            int i = 0;
            while (((token = strsep(&dstMac, ":")) != NULL) && (i < 6)) {
                rawHeader.linkHeader.ethernet.ether_dhost[i] = strtoul(token, NULL, 16);
                i++;
            }
        } else { // linux loopback dest is 00:00...
            memset(rawHeader.linkHeader.ethernet.ether_dhost, 0, ETHER_ADDR_LEN);
        }
    } else { // DLT_NULL
        rawHeader.linkHeader.loopback.type = 0x1E; // IPv6 traffic
    }

    // Construct v6 header
    rawHeader.ip6.ip6_vfc = 6 << 4;
    rawHeader.ip6.ip6_nxt = (sockType == SOCK_DGRAM) ? SOL_UDP : SOL_TCP;
    rawHeader.ip6.ip6_hlim = TTL;
    rawHeader.ip6.ip6_plen = htons(packet_send_size);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    int adret = getaddrinfo(srcIp.toUtf8().data(), NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    rawHeader.ip6.ip6_src = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;

    struct addrinfo *res1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    adret = getaddrinfo(dstIp.toUtf8().data(), NULL, &hints, &res1);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    rawHeader.ip6.ip6_dst = ((struct sockaddr_in6 *) res1->ai_addr)->sin6_addr;

    // pseudo header to compute checksum
    struct ipv6upper pHeader;
    memset(&pHeader, 0, sizeof(struct ipv6upper));
    pHeader.ip6_src = rawHeader.ip6.ip6_src;
    pHeader.ip6_dst = rawHeader.ip6.ip6_dst;
    pHeader.nextHeader = rawHeader.ip6.ip6_nxt;

    int nbBytes = packet_send_size;
    int padding = packet_send_size % 16;
    if (padding) {
        nbBytes = (packet_send_size / 16) * 16 + 16;
    }
    int checksumBufSize = sizeof(struct ipv6upper) + nbBytes;
    char* checksumPacket = static_cast<char*>(malloc(checksumBufSize));
    memset(checksumPacket, 0, checksumBufSize);

    if (sockType == SOCK_DGRAM) {
        struct sniff_udp* udp = static_cast<struct sniff_udp*>(static_cast<void*>(packet_send));
        udp->sport = htons(srcPort); // change udp src port
        pHeader.payload_len = udp->udp_length;
        memset(&(udp->udp_sum), 0, sizeof(u_short)); // checksum field to 0
        memcpy(checksumPacket, &pHeader, sizeof(struct ipv6upper));
        memcpy(checksumPacket + sizeof(struct ipv6upper), packet_send, packet_send_size);

        udp->udp_sum = ~(checksum(checksumPacket, sizeof(struct ipv6upper) + packet_send_size));
    } else {
        pHeader.payload_len = htonl(packet_send_size);

        struct sniff_tcp* tcp = static_cast<struct sniff_tcp*>(static_cast<void*>(packet_send));
        tcp->th_sport = htons(srcPort); // change source port
        memset(&(tcp->th_sum), 0, sizeof(u_short)); // checksum field to 0

        memcpy(checksumPacket, &pHeader, sizeof(struct ipv6upper));
        memcpy(checksumPacket + sizeof(struct ipv6upper), packet_send, packet_send_size);

        tcp->th_sum = ~(checksum(checksumPacket, sizeof(struct ipv6upper) + packet_send_size));
    }

    free(checksumPacket);

    // combine the rawHeader and packet in one contiguous block
    memcpy(buffer, &rawHeader, sizeof(struct rawComHeader));

    //write.lock(); // make sure one write at a time in buffer
    qDebug() << "raw write";
    raw->write(buffer, bufferSize);
    raw->waitForBytesWritten();
    qDebug() << "raw written";
    //write.unlock();
}

void RawSockets::packetTooBig(QString srcIp, QString dstIp, const char *packetBuffer) {
    IpResolver* r = IpResolver::getInstance();
    struct ip_mac_mapping map = r->getMapping(dstIp);

    if (map.interface == "") {
        qWarning() << "Mapping not found, cannot send packet!";
        return;
    }

    struct rawProcess* p = rawHelpers.value(map.interface);
    QProcess* raw = p->process;
    if (!raw || raw->state() != 2) {
        qFatal("No raw helper");
    }

    int linkLayerType = DLT_EN10MB;
#ifdef __APPLE__
    if (map.mac.isEmpty()) {
        linkLayerType = DLT_NULL;
    }
#endif

    quint32 packet_send_size = IPV6_MIN_MTU - sizeof(struct ipv6hdr) - sizeof(struct icmpv6TooBig);

    struct rawComHeader rawHeader;
    memset(&rawHeader, 0, sizeof(struct rawComHeader));
    rawHeader.payload_len = packet_send_size;

    if (linkLayerType == DLT_EN10MB) {
        rawHeader.linkHeader.ethernet.ether_type = htons(ETH_IPV6);
        // set source MAC
        const unsigned char* source_mac_addr;
        const char* if_name = map.interface.toUtf8().data();
#ifdef __APPLE__
        struct ifaddrs *ifap, *ifaptr;
        unsigned char *ptr = NULL;
        if (getifaddrs(&ifap) == 0) {
            for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
                if (!strcmp((ifaptr)->ifa_name, if_name) && (((ifaptr)->ifa_addr)->sa_family == AF_LINK)) {
                    ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)(ifaptr)->ifa_addr);
                    break;
                }
            }
            freeifaddrs(ifap);
        }
        source_mac_addr = ptr;
#elif __GNUC__
        struct ifreq ifr;
        size_t if_name_len=strlen(if_name);
        if (if_name_len<sizeof(ifr.ifr_name)) {
            memcpy(ifr.ifr_name,if_name,if_name_len);
            ifr.ifr_name[if_name_len]=0;
        } else {
            fprintf(stderr,"interface name is too long");
            exit(1);
        }
        // Open an IPv4-family socket for use when calling ioctl.
        int fd=socket(AF_INET,SOCK_DGRAM,0);
        if (fd==-1) {
            perror(0);
            exit(1);
        }
        // Obtain the source MAC address, copy into Ethernet header
        if (ioctl(fd,SIOCGIFHWADDR,&ifr)==-1) {
            perror(0);
            close(fd);
            exit(1);
        }

        source_mac_addr = (unsigned char*)ifr.ifr_hwaddr.sa_data;
        close(fd);
#endif

        memcpy(rawHeader.linkHeader.ethernet.ether_shost, source_mac_addr, ETHER_ADDR_LEN);
        // set dst mac
        char* dstMac = map.mac.toUtf8().data();
        if (!map.mac.isEmpty()) {
            char* token;
            int i = 0;
            while (((token = strsep(&dstMac, ":")) != NULL) && (i < 6)) {
                rawHeader.linkHeader.ethernet.ether_dhost[i] = strtoul(token, NULL, 16);
                i++;
            }
        } else { // linux loopback dest is 00:00...
            memset(rawHeader.linkHeader.ethernet.ether_dhost, 0, ETHER_ADDR_LEN);
        }
    } else { // DLT_NULL
        rawHeader.linkHeader.loopback.type = 0x1E; // IPv6 traffic
    }

    // Construct v6 header
    rawHeader.ip6.ip6_vfc = 6 << 4;
    rawHeader.ip6.ip6_nxt = SOL_ICMPV6;
    rawHeader.ip6.ip6_hlim = 64;
    rawHeader.ip6.ip6_plen = htons(packet_send_size + sizeof(struct icmpv6TooBig));

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    int adret = getaddrinfo(srcIp.toUtf8().data(), NULL, &hints, &res);
    if (adret) {
        qDebug() << gai_strerror(adret);
        return;
    }
    rawHeader.ip6.ip6_src = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;

    struct addrinfo *res1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    adret = getaddrinfo(dstIp.toUtf8().data(), NULL, &hints, &res1);
    if (adret) {
        qDebug() << gai_strerror(adret);
        return;
    }
    rawHeader.ip6.ip6_dst = ((struct sockaddr_in6 *) res1->ai_addr)->sin6_addr;

    // pseudo header to compute checksum
    struct ipv6upper pHeader;
    memset(&pHeader, 0, sizeof(struct ipv6upper));
    pHeader.ip6_src = rawHeader.ip6.ip6_src;
    pHeader.ip6_dst = rawHeader.ip6.ip6_dst;
    pHeader.nextHeader = rawHeader.ip6.ip6_nxt;
    pHeader.payload_len = rawHeader.ip6.ip6_plen;

    int nbBytes = packet_send_size;
    /*int padding = packet_send_size % 16;
    if (padding) {
        nbBytes = (packet_send_size / 16) * 16 + 16;
    }*/ // not sure if icmpv6 needs padding

    int checksumBufSize = sizeof(struct ipv6upper) + sizeof(struct icmpv6TooBig);// + nbBytes;
    char* checksumPacket = static_cast<char*>(malloc(checksumBufSize));
    memset(checksumPacket, 0, checksumBufSize);

    /* make icmpv6 packet too big header */
    struct icmpv6TooBig icmpheader;
    memset(&icmpheader, 0, sizeof(struct icmpv6TooBig));
    icmpheader.code = 0;
    icmpheader.type = 2;
    icmpheader.mtu = htonl(FVPN_MTU);
    memcpy(checksumPacket, &pHeader, sizeof(struct ipv6upper));
    memcpy(checksumPacket + sizeof(struct ipv6upper), &icmpheader, sizeof(struct icmpv6TooBig));
    //memcpy(checksumPacket + sizeof(struct ipv6upper) + sizeof(struct icmpv6TooBig), packetBuffer, packet_send_size);

    icmpheader.checksum = ~(checksum(checksumPacket, checksumBufSize));
    free(checksumPacket);

    int bufferSize = packet_send_size + sizeof(struct rawComHeader) + sizeof(struct icmpv6TooBig);
    char buffer[packet_send_size + sizeof(struct rawComHeader) + sizeof(struct icmpv6TooBig)];
    // combine the rawHeader and packet in one contiguous block
    memcpy(buffer, &rawHeader, sizeof(struct rawComHeader));
    memcpy(buffer + sizeof(struct rawComHeader), &icmpheader, sizeof(struct icmpv6TooBig));
    memcpy(buffer + sizeof(struct rawComHeader) + sizeof(struct icmpv6TooBig), packetBuffer, packet_send_size);

    qDebug() << "raw write";
    raw->write(buffer, bufferSize);
    raw->waitForBytesWritten();
    qDebug() << "raw written";
}
