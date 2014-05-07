#include "ph2phtp_parser.h"
#include "proxyserver.h"
#include <QStringList>
#include <QDebug>
#include <QThread>

PH2PHTP_Parser::PH2PHTP_Parser(QObject *parent) :
    QObject(parent)
{
}

bool PH2PHTP_Parser::parseControlPlane(const char *buf, QString friendUid) {
    QString packet(buf);

    QStringList list = packet.split("\r\n");

    qDebug() << "parser list!";
    qDebug() << list;

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
            }
        }

        if (hostname.isEmpty() || name.isEmpty() || type.isEmpty() || !port) {
            qDebug() << "ERROR: Bonjour packet wrong format";
            return false;
        }

        // TODO run proxy in new thread
        qDebug() << "Running new proxy!!";
        QThread* newProxyThread = new QThread();
        ProxyServer* newProxy = NULL;
        try {
            newProxy = new ProxyServer(friendUid, name, type, ".friendsvpn.", hostname, port);
        } catch (int i) {
            if (i == 1) {
                newProxyThread->deleteLater();
                return false; // proxy exists
            }
        }

        connect(newProxyThread, SIGNAL(started()), newProxy, SLOT(run()));
        connect(newProxyThread, SIGNAL(finished()), newProxyThread, SLOT(deleteLater()));
        newProxyThread->start();
        return true;
    }
}

/*bool PH2PHTP_Parser::sendData(const char *buf, int len, QString friendUid) {
    DataPlaneConnection* c = ConnectionInitiator::getDpConnection(friendUid);
    // make "DATA" packet

    // TODO
    c->sendBytes(buf, len);
}*/
