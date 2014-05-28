#ifndef BONJOURRECORD_H
#define BONJOURRECORD_H
#include <QString>
#include <QList>
#include "dns_sd.h"

/**
 * @brief The BonjourRecord class holds the details of a specific bonjour record.
 */
class BonjourRecord
{
public:
    QString serviceName;
    QString registeredType;
    QString replyDomain;

    // resolved information, using BonjourResolver
    QString hostname;
    QList<QString> ips; // list of ips, begins with ipv6s and ends with v4s
    quint16 port;
    bool resolved;
    QByteArray md5; /* md5 computed by the bonjourResolver */
    QString txt;

    BonjourRecord() {}

    BonjourRecord(const QString &name, const QString &regType, const QString &domain)
        : serviceName(name), registeredType(regType), replyDomain(domain)
    {
        resolved = false;
    }
    BonjourRecord(const char *name, const char *regType, const char *domain)
    {
        resolved = false;
        serviceName = QString::fromUtf8(name);
        registeredType = QString::fromUtf8(regType);
        replyDomain = QString::fromUtf8(domain);
    }
    BonjourRecord(const QString &name, const QString &regType, const QString &domain,
                  const QString &hostname, const QList<QString> &ips, quint16 port, const QString& txt)
    : serviceName(name), registeredType(regType), replyDomain(domain), hostname(hostname), ips(ips),
      port(port), txt(txt)
    {
        resolved = true;
    }

    bool operator==(const BonjourRecord &other) const {
        return serviceName == other.serviceName
               && registeredType == other.registeredType
               && replyDomain == other.replyDomain;
    }
};

#endif // BONJOURRECORD_H
