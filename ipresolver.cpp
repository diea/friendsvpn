#include "ipresolver.h"
IpResolver* IpResolver::instance = NULL;

IpResolver::IpResolver() :
    QObject()
{
}

IpResolver* IpResolver::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (instance == NULL) {
        instance = new IpResolver();
    }
    mutex.unlock();
    return instance;
}

void IpResolver::addMapping(QString ip, QString mac, QString interface) {
    struct ip_mac_mapping newmap;
    newmap.interface = interface;
    newmap.mac = mac;
    newmap.ip = ip;

    mutex.lock();
    mappings.insert(ip, newmap);
    mutex.unlock();
}

struct ip_mac_mapping IpResolver::getMapping(QString ip) {
    mutex.lock();
    if (mappings.contains(ip)) {
        mutex.unlock();
        return mappings.value(ip);
    } else {
        // do neighbor solicitation

        // fail
        mutex.unlock();
        struct ip_mac_mapping nullMapping;
        nullMapping.mac = "NULL";
        return nullMapping;
    }
}
