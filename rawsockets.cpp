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
                qDebug() << "waiting for new raw socket start";
                r->process->waitForStarted();
                qDebug() << "insert!";
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
    qDebug() << "waiting for new raw socket start";
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
                    qDebug() << "waiting for new raw socket start";
                    r->process->waitForStarted();
                    qDebug() << "insert!";
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
    qDebug() << "waiting for new raw socket start";
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
    qDebug() << "got here";
    struct ip_mac_mapping map = r->getMapping(dstIp);
    qDebug() << "got here";
    struct rawProcess* p = rawHelpers.value(map.interface);
    qDebug() << "got here";
    QProcess* raw = p->process;
    qDebug() << "got here";
    if (!raw || raw->state() != 2) {
        qDebug() << "raw state is " << raw->state() << "and interface " << map.interface;
        qFatal("No raw helper");
    }
    qDebug() << "got here";

    int linkLayerType = DLT_EN10MB;
#ifdef __APPLE__
    if (map.mac.isEmpty()) {
        linkLayerType = DLT_NULL;
    }
#endif
    qDebug() << "got here";

    char* buffer = static_cast<char*>(malloc(packet_send_size + sizeof(struct rawComHeader)));
    memcpy(buffer + sizeof(struct rawComHeader), transAndPayload, packet_send_size);

    char* packet_send = buffer + sizeof(struct rawComHeader);

    struct rawComHeader rawHeader;
    memset(&rawHeader, 0, sizeof(struct rawComHeader));
    rawHeader.payload_len = packet_send_size;
    qDebug() << "got here";

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
        qDebug() << "got in here";
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
        // Obtain the source MAC address, copy into Ethernet header and ARP request.
        if (ioctl(fd,SIOCGIFHWADDR,&ifr)==-1) {
            perror(0);
            close(fd);
            exit(1);
        }
        if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
            fprintf(stderr,"not an Ethernet interface\n");
        }

        source_mac_addr = (unsigned char*)ifr.ifr_hwaddr.sa_data;
        close(fd);
        qDebug() << "got out of here";
#endif

        memcpy(rawHeader.linkHeader.ethernet.ether_shost, source_mac_addr, ETHER_ADDR_LEN);

        // set dst mac
        char* dstMac = map.mac.toUtf8().data();
        if (map.mac.isEmpty()) { // "00:00:00:00:00:00"
            memset(dstMac, 0, ETHER_ADDR_LEN);
        }
        qDebug() << "got here";

        char* token = static_cast<char*>(malloc(sizeof(char)));
        int i = 0;
        while (((token = strsep(&dstMac, ":")) != NULL) && (i < 6)) {
            rawHeader.linkHeader.ethernet.ether_dhost[i] = strtoul(token, NULL, 16);
            //printf("%x\n", header.ether_dhost[i]);
            i++;
        }
        free(token);
        qDebug() << "got here";

    } else { // DLT_NULL
        rawHeader.linkHeader.loopback.type = 0x1E;
    }
    qDebug() << "got here";

    // Construct v6 header
    rawHeader.ip6.ip6_vfc = 6 << 4;
    rawHeader.ip6.ip6_nxt = (sockType == SOCK_DGRAM) ? SOL_UDP : SOL_TCP;
    rawHeader.ip6.ip6_hlim = TTL;
    rawHeader.ip6.ip6_plen = packet_send_size;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    int adret = getaddrinfo(srcIp.toUtf8().data(), NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    rawHeader.ip6.ip6_src = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
    adret = getaddrinfo(dstIp.toUtf8().data(), NULL, &hints, &res);
    if (adret) {
        fprintf(stderr, "%s\n", gai_strerror(adret));
        exit(0);
    }
    rawHeader.ip6.ip6_dst = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;

    // pseudo header to compute checksum
    struct ipv6upper pHeader;
    memset(&pHeader, 0, sizeof(struct ipv6upper));
    pHeader.ip6_src = rawHeader.ip6.ip6_src;
    pHeader.ip6_dst = rawHeader.ip6.ip6_dst;
    pHeader.nextHeader = rawHeader.ip6.ip6_nxt;

    struct sniff_udp* udp = NULL;

    int nbBytes = packet_send_size;
    int padding = packet_send_size % 16;

    if (padding) {
        nbBytes = (packet_send_size / 16) * 16 + 16;
    }
    int checksumBufSize = sizeof(struct ipv6upper) + nbBytes;
    char* checksumPacket = static_cast<char*>(malloc(checksumBufSize));
    qDebug() << "got here";

    if (sockType == SOCK_DGRAM) {
        udp = static_cast<struct sniff_udp*>(static_cast<void*>(packet_send));
        pHeader.payload_len = udp->udp_length; // wait, same number of bits ? TODO
        memcpy(checksumPacket, &pHeader, sizeof(struct ipv6upper));

        // change src_port
        udp->sport = htons(srcPort);
    } else {
        pHeader.payload_len = htonl(packet_send_size);

        struct sniff_tcp* tcp = static_cast<struct sniff_tcp*>(static_cast<void*>(packet_send));
        tcp->th_sport = htons(srcPort); // change source port
        memset(&(tcp->th_sum), 0, sizeof(u_short)); // checksum field to 0

        memcpy(checksumPacket, &pHeader, sizeof(struct ipv6upper));
        memcpy(checksumPacket + sizeof(struct ipv6upper), packet_send, packet_send_size);

        tcp->th_sum = ~(checksum(checksumPacket, sizeof(struct ipv6upper) + packet_send_size));
        free(checksumPacket);
    }
    qDebug() << "got here";

    // combine the rawHeader and packet in one contiguous block
    memcpy(buffer, &rawHeader, sizeof(struct rawComHeader));

    write.lock(); // make sure one write at a time in buffer
    raw->write(buffer, packet_send_size + sizeof(struct rawComHeader));
    raw->waitForBytesWritten();
    write.unlock();
}
