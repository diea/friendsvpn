#include "ipresolver.h"
#include <QProcess>
#include <QDebug>
#include <QStack>
#include <QThread>
IpResolver* IpResolver::instance = NULL;

IpResolver::IpResolver() :
    QObject()
{
#ifdef __APPLE__
    addMapping("::1", "", "lo0");
#elif __GNUC__
    addMapping("::1", "", "lo");
#endif
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
    qDebug() << "add mapping " << ip << mac << interface;
    newmap.interface = interface;
    newmap.mac = mac;
    newmap.ip = ip;

    mutex.lock();
    mappings.insert(ip, newmap);
    qDebug() << "insert was done!";
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
        char buf[3000];
        int length;
        while (ndp.waitForReadyRead(500)) {
            while ((length = ndp.readLine(buf, 3000))) {
                QString curLine(buf);
                QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                qDebug() << list.at(0);
                if (list.at(0).contains(ip)) {
                    this->addMapping(ip, list.at(1), list.at(2));
                    qDebug() << "add mapping was done!";
                    ndp.close();
                    return getMapping(ip);
                }
            }
        }
        ndp.close();
#elif __GNUC__
        ndp.start("ip -6 neigh");
        ndp.waitForReadyRead();
        char buf[3000];
        int length;
        while (ndp.waitForReadyRead(500)) {
            while ((length = ndp.readLine(buf, 3000))) {
                QString curLine(buf);
                QStringList list = curLine.split(" ", QString::SkipEmptyParts);
                if (list.at(0) == ip) {
                    this->addMapping(ip, list.at(4), list.at(2));
                    ndp.close();
                    return getMapping(ip);
                }
            }
        }
        ndp.close();
#endif
        qDebug() << "going into ifconfig";
        // is it a local address ?
        QProcess testIfconfig;
        testIfconfig.start("/sbin/ifconfig");
        testIfconfig.waitForReadyRead();
        while ((length = testIfconfig.readLine(buf, 3000))) {
            QString curLine(buf);
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            foreach (QString value, list) {
                if (value.contains(ip)) {
#ifdef __APPLE__
                    this->addMapping(ip, "", "lo0");
#elif __GNUC__
                    this->addMapping(ip, "", "lo");
#endif
                    testIfconfig.close();
                    return getMapping(ip);
                }
            }
        }
        testIfconfig.close();

        struct ip_mac_mapping nullMapping;
        // fail
        return nullMapping;
    }
}

QStack<QString> IpResolver::getActiveInterfaces() {
    // we should list all interfaces but for now we just go with the default interface and
    // the loopback default
    QStack<QString> activeIfaces;
#ifdef __APPLE__
    activeIfaces.push("lo0");
#elif __GNUC__
    activeIfaces.push("lo");
#endif
    activeIfaces.push(getDefaultInterface());
    return activeIfaces;
}

QString IpResolver::getDefaultInterface() {
#ifdef __APPLE__
    QProcess netstat;
    netstat.start("netstat -nr");
    netstat.waitForReadyRead();
    char buf[3000];
    int length;
    while ((length = netstat.readLine(buf, 3000))) {
        buf[length - 1] = '\0'; // remove "\n"
        QString curLine(buf);
        if (curLine.startsWith("default")) {
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            if (list.at(1).contains(":")) {
                netstat.close();
                return list.at(3);
            }
        }
    }
    netstat.close();
    qWarning() << "The host has no IPv6 default route!";
    exit(-1);
#elif __GNUC__
    QProcess route;
    route.start("route -n6");
    route.waitForReadyRead();
    char buf[3000];
    int length;
    while ((length = route.readLine(buf, 3000))) {
        buf[length - 1] = '\0'; // remove "\n"
        QString curLine(buf);
        if (curLine.startsWith("::/0")) {
            QStringList list = curLine.split(" ", QString::SkipEmptyParts);
            if (list.at(2) != "::") {
                route.close();
                return list.at(6);
            }
        }
    }
    route.close();
    qWarning() << "The host has no IPv6 default route!";
    exit(-1);
#endif
}
