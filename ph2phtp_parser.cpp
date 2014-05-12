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

    return false;
}
