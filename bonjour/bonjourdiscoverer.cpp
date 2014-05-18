#include "bonjourdiscoverer.h"
QHash<QString, BonjourRecord*> BonjourDiscoverer::recordHashes;

BonjourDiscoverer::BonjourDiscoverer(QObject *parent)
{
    setParent(parent);
    this->qSql = BonjourSQL::getInstance();
    dnsref = 0;
}

BonjourDiscoverer* BonjourDiscoverer::instance = NULL;
BonjourDiscoverer* BonjourDiscoverer::getInstance(QObject *parent) {
    if (!instance) {
        instance = new BonjourDiscoverer(parent);
        qDebug() << "bonjourdiscoverer thread id" << QThread::currentThreadId();
    }
    return instance;
}

BonjourDiscoverer::~BonjourDiscoverer()
{
    if (dnsref) {
        DNSServiceRefDeallocate(dnsref);
        dnsref = 0;
    }
}

void BonjourDiscoverer::discoverServices() {
    QString name = "_services._dns-sd._udp";
    // Browse with QU bit since we have no prior knowledge. This avoids spamming others.
    DNSServiceErrorType err = DNSServiceBrowse(&dnsref, 0, 0, name.toUtf8().constData(), 0,
                                               reply, this);
    if (err != kDNSServiceErr_NoError) {
        emit error(err);
    } else {
        int sockfd = DNSServiceRefSockFD(dnsref);
        if (sockfd == -1) {
            emit error(kDNSServiceErr_Invalid);
        } else {
            bonjourSocket = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
            connect(bonjourSocket, SIGNAL(activated(int)), this, SLOT(bonjourSocketReadyRead()));
        }
    }
}

void BonjourDiscoverer::reply(DNSServiceRef dnsref, DNSServiceFlags flags,
                         quint32 , DNSServiceErrorType errorCode,
                         const char *serviceName, const char *regType,
                         const char *replyDomain, void *context) {
    BonjourDiscoverer *serviceBrowser = static_cast<BonjourDiscoverer *>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit serviceBrowser->error(errorCode);
    } else {
        QString reg = QString(regType);
        reg.truncate(4); // remove everything after ._tcp or ._udp
        QString service = QString(serviceName) + "." + reg;
        if (flags & kDNSServiceFlagsAdd) {
            if (!serviceBrowser->availableServices.contains(service)) {
                BonjourBrowser* bb = new BonjourBrowser();
                bb->browseForServiceType(service);
                serviceBrowser->availableServices.insert(service, bb);
                qDebug() << "Adding " + QString(service);
                // Add to database
                serviceBrowser->qSql->insertService(serviceName, reg.right(3));
            }
        } else {
            delete serviceBrowser->availableServices.value(service);
            serviceBrowser->availableServices.remove(service);

            // TODO serviceBrowser->qSql->removeService(serviceName, reg.right(3));
            qDebug() << "Removing " + QString(service);
        }
    }
}

void BonjourDiscoverer::bonjourSocketReadyRead() {
    DNSServiceErrorType err = DNSServiceProcessResult(dnsref);
    if (err != kDNSServiceErr_NoError)
        emit error(err);
}

QList < BonjourRecord* > BonjourDiscoverer::getAllActiveRecords() {
    QList < BonjourRecord* > list;
    QHashIterator<QString, BonjourBrowser* > i(this->availableServices);
    //qDebug() << "Got the available services";
    //qDebug() << instance->availableServices; // attention use instance & not this here
    while (i.hasNext()) {
        //qDebug() << "Got new BonjourBrowser, building list";
        BonjourBrowser* brw = i.next().value();
        //qDebug() << "Appending";
        list.append(brw->bonjourRecords);
    }
    //qDebug() << "Return list";
    return list;
}












