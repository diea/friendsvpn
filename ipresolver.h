#ifndef IPRESOLVER_H
#define IPRESOLVER_H

#include <QObject>
#include <QHash>
#include <QMutex>

/**
 * @brief The ipResolver class will hold the local mappings of MAC - IPs. It will resolve the MAC
 * of a given IP if needed by checking the neighbor cache or if that fails by trying a sort
 * of inverse neighbor discovery.
 */

struct ip_mac_mapping {
    QString ip;
    QString mac; // mac will be empty if loopback interface or no available mac
    QString interface;
};

class IpResolver : public QObject
{
    Q_OBJECT
private:
    explicit IpResolver();
    QMutex mutex;

    QHash<QString, struct ip_mac_mapping> mappings;

    static IpResolver* instance;
    QString defaultInterface;

public:
    static IpResolver* getInstance();

    void addMapping(QString ip, QString mac, QString interface);
    /**
     * @brief getMapping
     * @param ip
     * @return a default constructed ip_mac_mapping if ip is not in the kernel's neighbor memory
     */
    struct ip_mac_mapping getMapping(QString ip);

    /**
     * @brief getActiveInterfaces gives a list of active interfaces. This would be used by pcap
     * to known on which interfaces to capture
     * @return
     */
    QStack<QString> getActiveInterfaces();

    /**
     * @brief getDefaultInterface gets the default interface (the active interface for a user
     * that is NOT multihomed)
     * @return
     */
    static QString getDefaultInterface();
};

#endif // IPRESOLVER_H
