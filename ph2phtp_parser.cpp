#include "ph2phtp_parser.h"

PH2PHTP_Parser::PH2PHTP_Parser(QObject *parent) :
    QObject(parent)
{
}

bool PH2PHTP_Parser::parseControlPlane(const char *buf, QString friendUid) {
    QString packet(buf);

    QStringList list = packet.split("\r\n");

    if (!list.at(list.length() - 1).isEmpty())
        return false;

    if (list.at(0) == "BONJOUR") {
        QString hostname;
        QString name;
        QString type;
        int port = 0;

        for (int i = 1; i < list.length() - 1; i++) {
            QStringList keyValuePair = list.at(i).split(":");
            if (keyValuePair.at(0) == "Hostname") {
                hostname = keyValuePair.at(1);
            } else if (keyValuePair.at(0) == "Name") {
                name = keyValuePair.at(1);
            } else if (keyValuePair.at(0) == "Type") {
                type = keyValuePair.at(1);
            } else if (keyValuePair.at(0) == "Port") {
                port = keyValuePair.at(1).toInt();
            } else
                return false; // wrong format of packet
        }

        if (hostname.isEmpty() || name.isEmpty() || type.isEmpty() || !port) {
            return false;
        }

        // TODO run proxy in new thread
        QThread* newProxyThread = new QThread();
        Proxy* newProxy = new Proxy(friendUid, name, type, ".friendsvpn.", hostname, port);
        connect(newProxyThread, SIGNAL(started()), newProxy, SLOT(run));
        connect(newProxyThread, SIGNAL(finished()), newProxyThread, SLOT(deleteLater()));
        newProxyThread->start();
    }
}
