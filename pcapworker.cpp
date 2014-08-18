#include "pcapworker.h"
#include "proxy.h"
#include <QThread>
PcapWorker::PcapWorker(QStringList args, Proxy* p, QThread* t) :
    p(p), args(args), t(t)
{
    pos = 0;
    remaining = 0;
    pcap = NULL;
}

PcapWorker::~PcapWorker()
{
    qDebug() << "Closing pcapListen" << pcap->arguments();
    if (t) t->exit(0);

    if (pcap) {
        pcap->disconnect();
        pcap->terminate();
        connect(pcap, SIGNAL(finished(int)), pcap, SLOT(deleteLater()));
    }
    pcap = NULL;
}

void PcapWorker::run() {
    pcap = new QProcess(0);
    connect(pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));

    pcap->start(QCoreApplication::applicationDirPath() +QString(HELPERPATH) + "pcapListen", args);
    qDebug() << "pcapListen" << args << "runs in thread ID" << QThread::currentThreadId();
    pcap->waitForStarted();
    pcap->closeWriteChannel();
    pcap->setReadChannel(QProcess::StandardOutput);

    qDebug() << "Pcap ready, waiting for bytes";
    while (pcap->waitForReadyRead(-1)) {
        qDebug() << "Before reading header PCAP has" << pcap->bytesAvailable() << "bytes available";

        while (pcap->bytesAvailable() > qint64(sizeof(pcapComHeader))) {
            if (remaining <= 0) {
                char pcapHeadChar[sizeof(struct pcapComHeader)];
                pcap->read(pcapHeadChar, sizeof(struct pcapComHeader));

                memset(&pcapHeader, 0, sizeof(struct pcapComHeader));
                memcpy(&pcapHeader, pcapHeadChar, sizeof(struct pcapComHeader));
                memset(&packet, 0, MAX_PACKET_SIZE);
                remaining = pcapHeader.len;
                if (!(remaining >= 0 && remaining <= 65536)) {
                    qWarning() << "Remaining bytes was not between 0 and 65536 !";
                    continue;
                }
                pos = 0;
            }

            qint64 bytesAv = pcap->bytesAvailable();

            if (bytesAv >= remaining) {
                pcap->read(packet + pos, remaining);
                pos += remaining;
                remaining = 0;
            } else {
                pcap->read(packet + pos, bytesAv);
                pos += bytesAv;
                remaining -= bytesAv;
                qDebug() << "Packet read remaining" << remaining;
                qDebug() << "Packet read pos:" << pos;
                continue;
            }

            if (p->port != p->listenPort) {
                // first 16 bits = source Port of UDP and TCP
                quint16* dstPort = static_cast<quint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
                *dstPort = htons(p->listenPort); // restore the original port
            }

            qDebug() << "Pcap receives a packet of" << pcapHeader.len << "bytes";
            p->receiveBytes(packet, pcapHeader.len, p->sockType, pcapHeader.ipSrcStr);
        }
    }
}

void PcapWorker::pcapFinish(int exitCode) {
    qWarning() << "pcap exited with exit code " << exitCode;
}
