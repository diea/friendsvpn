#include "bonjourbrowser.h"
#include "bonjourdiscoverer.h"
#include <QApplication>

BonjourBrowser::BonjourBrowser(QObject *parent)
{
    setParent(parent);
    this->qSql = DatabaseHandler::getInstance();
    dnsref = 0;
    bonjourSocket = 0;
}

BonjourBrowser::~BonjourBrowser()
{
    if (dnsref) {
        DNSServiceRefDeallocate(dnsref);
        dnsref = 0;
    }
}

void BonjourBrowser::browseForServiceType(const QString &serviceType)
{
    DNSServiceErrorType err = DNSServiceBrowse(&dnsref, 0, 0, serviceType.toUtf8().constData(), 0,
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

void BonjourBrowser::reply(DNSServiceRef , DNSServiceFlags flags,
                                               quint32 , DNSServiceErrorType errorCode,
                                               const char *serviceName, const char *regType,
                                               const char *replyDomain, void *context)
{
    BonjourBrowser *serviceBrowser = static_cast<BonjourBrowser *>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit serviceBrowser->error(errorCode);
    } else if (strncmp(serviceName, "fvpn_", 5) != 0) { /* we ignore if starts with friendsvpn
                                                                 because those are records the app
                                                                 has registered */
        BonjourRecord* bonjourRecord = new BonjourRecord(serviceName, regType, replyDomain);

        if (flags & kDNSServiceFlagsAdd) {
            if (!serviceBrowser->bonjourRecords.contains(bonjourRecord))
                serviceBrowser->bonjourRecords.append(bonjourRecord);
        } else { // delete the record
            QList<BonjourRecord*>::iterator it;
            int bjrIndex = 0;
            // iterate through list to find the record to delete
            foreach (BonjourRecord* oldBonjourRecord, serviceBrowser->bonjourRecords) {
                if (*oldBonjourRecord == *bonjourRecord) {
                    serviceBrowser->bonjourRecords.removeAt(bjrIndex);
                    if (oldBonjourRecord->resolved) {
                        // delete record from bonjour discoverer global list
                        BonjourDiscoverer::recordHashes.remove(oldBonjourRecord->md5);
                    }
                    delete oldBonjourRecord;
                }
                bjrIndex++;
            }
            delete bonjourRecord;
        }
        if (!(flags & kDNSServiceFlagsMoreComing)) {
            emit serviceBrowser->currentBonjourRecordsChanged(serviceBrowser->bonjourRecords);
        }
        if (flags & kDNSServiceFlagsAdd) {
            BonjourResolver *resolver = new BonjourResolver(bonjourRecord);
            connect(resolver, SIGNAL(resolved(BonjourRecord*)),
                    serviceBrowser, SLOT(recordIsReady(BonjourRecord*)));
            resolver->resolve(); // resolver object is deleted in itself after signal resolved triggered
        }
    }
}

void BonjourBrowser::recordIsReady(BonjourRecord* rec) {
    qDebug() << "Record is ready:" << rec->hostname << " " <<
             rec->registeredType << " IP " <<  rec->ips << " " << rec->port;
    BonjourResolver* resolver = qobject_cast<BonjourResolver*>(sender());
    qDebug() << "Delete resolver";
    delete resolver;
}

void BonjourBrowser::bonjourSocketReadyRead() {
    DNSServiceErrorType err = DNSServiceProcessResult(dnsref);
    if (err != kDNSServiceErr_NoError)
        emit error(err);
}
