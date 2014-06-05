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
    pcap = new QProcess(this);
    connect(pcap, SIGNAL(finished(int)), this, SLOT(pcapFinish(int)));

    UnixSignalHandler* u = UnixSignalHandler::getInstance();
    u->addQProcess(pcap); // add pcap to be killed when main is killed

    pcap->start(QString(HELPERPATH) + "pcapListen", args);
    pcap->waitForStarted();

    while (pcap->waitForReadyRead()) {
        qDebug() << "Before reading header PCAP has" << pcap->bytesAvailable() << "bytes available";
        if (remaining <= 0) {
            if (pcap->bytesAvailable() < qint64(sizeof(pcapComHeader))) {
                qDebug() << "PCAP header was not available, not enough bytes to be read";
                continue; // wait for more!
            }
            pcap->read(static_cast<char*>(static_cast<void*>(&pcapHeader)), sizeof(struct pcapComHeader));
            packet = static_cast<char*>(malloc(pcapHeader.len * sizeof(char)));
            remaining = pcapHeader.len;
            pos = 0;
        }
        qint64 bytesAv = pcap->bytesAvailable();

        qDebug() << "PCAP has" << pcap->bytesAvailable() << "bytes available";
        qDebug() << "PCAP Header demands" << pcapHeader.len << "bytes";
        /* should not happen since we write everything in one fwrite in buffer */
        if (bytesAv >= remaining) {
            pcap->read(packet + pos, remaining);
            remaining = 0;
        } else {
            pos += bytesAv;
            remaining -= bytesAv;
            pcap->read(packet + pos, bytesAv);
            qDebug() << "Not enough bytes yet, reading must continue";
            continue;
        }

        if (p->port != p->listenPort) {
            // first 16 bits = source Port of UDP and TCP
            quint16* dstPort = static_cast<quint16*>(static_cast<void*>(packet + 2)); // second 16 bits dstPort (or + 2 bytes)
            *dstPort = htons(p->listenPort); // restore the original port
        }

        QString ipSrc(pcapHeader.ipSrcStr);

        qDebug() << "Receiving" << pcapHeader.len << "bytes from PCAP!";
        p->receiveBytes(packet, pcapHeader.len, p->sockType, ipSrc);

        // TODO free packet
    }
}

void PcapWorker::pcapFinish(int exitCode) {
    qWarning() << "pcap exited with exit code " << exitCode;
    QThread::currentThread()->exit(0);
}
