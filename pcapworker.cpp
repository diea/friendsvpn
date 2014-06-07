#include "pcapworker.h"
#include "proxy.h"
#include <QThread>
PcapWorker::PcapWorker(QStringList args, Proxy* p, QObject *parent) :
    QObject(parent),  p(p), args(args)
{
    pos = 0;
    remaining = 0;
}

void PcapWorker::run() {
    QProcess pcap;
    connect(&pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));

    pcap.start(QString(HELPERPATH) + "pcapListen", args);
    pcap.waitForStarted();
    pcap.closeWriteChannel();
    pcap.setReadChannel(QProcess::StandardOutput);

    while (pcap.waitForReadyRead(-1)) {
        qDebug() << "Waiting for ready read";
        qDebug() << "Before reading header PCAP has" << pcap.bytesAvailable() << "bytes available";

        while (pcap.bytesAvailable() > qint64(sizeof(pcapComHeader))) {
            if (remaining <= 0) {
                char pcapHeadChar[sizeof(struct pcapComHeader)];
                pcap.read(pcapHeadChar, sizeof(struct pcapComHeader));

                memset(&pcapHeader, 0, sizeof(struct pcapComHeader));
                memcpy(&pcapHeader, pcapHeadChar, sizeof(struct pcapComHeader));
                memset(&packet, 0, MAX_PACKET_SIZE);
                remaining = pcapHeader.len;
                pos = 0;
            }

            qint64 bytesAv = pcap.bytesAvailable();

            if (bytesAv >= remaining) {
                pos += remaining;
                pcap.read(packet + pos, remaining);
                remaining = 0;
            } else {
                pcap.read(packet + pos, bytesAv);
                pos += bytesAv;
                remaining -= bytesAv;
                continue;
            }

            if ((pcapHeader.len > FVPN_MTU) && (pos > IPV6_MIN_MTU)) {
                qDebug() << "Got packet of size" << remaining << "sending icmpv6 too big";

                RawSockets* s = RawSockets::getInstance();
                s->packetTooBig(p->listenIp, pcapHeader.ipSrcStr, packet);
                continue;
            }


            if (p->port != p->listenPort) {
                // first 16 bits = source Port of UDP and TCP
                quint16* dstPort = static_cast<quint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
                *dstPort = htons(p->listenPort); // restore the original port
            }

            p->receiveBytes(packet, pcapHeader.len, p->sockType, pcapHeader.ipSrcStr);
        }
    }
}

void PcapWorker::pcapFinish(int exitCode) {
    qWarning() << "pcap exited with exit code " << exitCode;
    QThread::currentThread()->exit(0);
}
