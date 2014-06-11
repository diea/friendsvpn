#ifndef PCAPWORKER_H
#define PCAPWORKER_H

#include <QObject>
#include <QProcess>
#include "helpers/raw_structs.h"
#include "unixsignalhandler.h"
#include "config.h"

class Proxy;
class PcapWorker : public QObject
{
    Q_OBJECT
private:
    Proxy* p;
    QStringList args; // args for the pcap QProcess
    char packet[MAX_PACKET_SIZE];
    qint64 pos;
    qint64 remaining;
    struct pcapComHeader pcapHeader;
    QProcess* pcap;
public:
    /**
     * @brief PcapWorker handles a pcap process inside a QThread and hands the data back to
     * its associated proxy (param p) by using the receiveBytes function.
     * @param p
     * @param parent
     */
    explicit PcapWorker(QStringList args, Proxy* p);
    ~PcapWorker();
signals:

public slots:
    void run();
    void pcapFinish(int exitCode);
};

#endif // PCAPWORKER_H
