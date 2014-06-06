#include "pcapworker.h"
#include "proxy.h"
#include <QThread>
PcapWorker::PcapWorker(QStringList args, Proxy* p, QObject *parent) :
    QObject(parent),  p(p), args(args)
{
    pos = 0;
    remaining = 0;
    pcapHeader = NULL;
}

void PcapWorker::run() {
    QProcess pcap;
    connect(&pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));

    //UnixSignalHandler* u = UnixSignalHandler::getInstance();
    //u->addQProcess(pcap); // add pcap to be killed when main is killed

    pcap.start(QString(HELPERPATH) + "pcapListen", args);
    pcap.waitForStarted();

    while (1) {
        pcap.waitForReadyRead(400);
        qDebug() << "Before reading header PCAP has" << pcap.bytesAvailable() << "bytes available";
        if (remaining <= 0) {
            if (pcap.bytesAvailable() < qint64(sizeof(pcapComHeader))) {
                qDebug() << "PCAP header was not available, not enough bytes to be read";
                continue; // wait for more!
            }
            pcapHeader = static_cast<struct pcapComHeader *>(malloc(sizeof(struct pcapComHeader)));
            char pcapHeadChar[5000];
            pcap.read(pcapHeadChar, sizeof(struct pcapComHeader));
            memcpy(pcapHeader, pcapHeadChar, sizeof(struct pcapComHeader));
            memset(&packet, 0, MAX_PACKET_SIZE);
            remaining = pcapHeader->len;
        }

        qint64 bytesAv = pcap.bytesAvailable();
        if (bytesAv >= remaining) {
            pcap.read(packet + pos, remaining);
            remaining = 0;
            pos = 0;
        } else {
            pcap.read(packet + pos, bytesAv);
            pos += bytesAv;
            remaining -= bytesAv;
            continue;
        }

        if (p->port != p->listenPort) {
            // first 16 bits = source Port of UDP and TCP
            quint16* dstPort = static_cast<quint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
            *dstPort = htons(p->listenPort); // restore the original port
        }

        QString ipSrc(pcapHeader->ipSrcStr);

        p->receiveBytes(packet, pcapHeader->len, p->sockType, ipSrc);

        free(pcapHeader);
        pcapHeader = NULL;
    }
}

void PcapWorker::pcapFinish(int exitCode) {
    qWarning() << "pcap exited with exit code " << exitCode;
    QThread::currentThread()->exit(0);
}
