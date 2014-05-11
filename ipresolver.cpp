#include "ipresolver.h"
#include <QProcess>

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
        mutex.unlock();
        // check kernel neighbor cache
        QProcess ndp;
#ifdef __APPLE__
        ndp.start("ndp -an");
        ndp.waitForReadyRead();
        char buf[3000];
        int length;
        while ((length = ndp.readLine(buf, 3000))) {
            QString curLine(buf);
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            if (list.at(0) == ip) {
                this->addMapping(ip, list.at(1), list.at(3));
                ndp.close();
                return getMapping(ip);
            }
        }
        ndp.close();
#elif __GNUC__
        ndp.start("ip -6 neigh");
        ndp.waitForReadyRead();
        char buf[3000];
        int length;
        while ((length = ndp.readLine(buf, 3000))) {
            QString curLine(buf);
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            if (list.at(0) == ip) {
                this->addMapping(ip, list.at(4), list.at(2));
                ndp.close();
                return getMapping(ip);
            }
        }
        ndp.close();
#endif
        struct ip_mac_mapping nullMapping;
        // fail
        return nullMapping;
    }
}
