#include "bonjoursql.h"

BonjourSQL::BonjourSQL(QObject *parent) :
    QObject(parent)
{
    while (!db.open()) {
        initDB();
        if (!db.open()) {
            QMessageBox msgBox;
            msgBox.setText(db.lastError().text());
            msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Abort);
            msgBox.setDefaultButton(QMessageBox::Retry);
            int ret = msgBox.exec();
            switch (ret) {
                case QMessageBox::Retry:
                    continue;
                case QMessageBox::Abort:
                    qDebug() << "Abort !!";
                    exit(0);
            }
        }
    }
}

BonjourSQL::~BonjourSQL() {
}

void BonjourSQL::uidOK() {
    while (uid == NULL) {
        QMessageBox msgBox;
        msgBox.setText("Your facebook ID was not determined, could you refresh the facebook page and click retry ?");
        msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Abort);
        msgBox.setDefaultButton(QMessageBox::Retry);
        int ret = msgBox.exec();
        switch (ret) {
            case QMessageBox::Retry:
                continue;
            case QMessageBox::Abort:
                qDebug() << "Abort !!";
                exit(0);
                break;
        }
    }
}

void BonjourSQL::initDB() {
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(DBHOST);
    db.setDatabaseName(DBNAME);
    db.setUserName(DBUSER);
    db.setPassword(DBPASS);
}

bool BonjourSQL::insertService(QString name, QString trans_prot) {
    QSqlQuery query = QSqlQuery(db);
    query.prepare("INSERT INTO Service VALUES(?, ?, ?)");
    query.bindValue(0, name);
    query.bindValue(1, uid);
    query.bindValue(2, trans_prot);
    query.exec();
    // test query
    qDebug() << "SQL SERVICE: " << "INSERT INTO Service VALUES(" + name +", "+uid+", "+trans_prot+")";
    qDebug() << "Insert Service error" << query.lastError();
    return true;
}

// TODO test this
bool BonjourSQL::removeService(QString name, QString trans_prot) {
    QSqlQuery query = QSqlQuery(db);
    query.prepare("DELETE FROM Authorized_user WHERE Device_Service_name = ? AND Device_Service_User_uid = ? AND Device_Service_Trans_prot = ?");
    query.bindValue(0, name);
    query.bindValue(1, uid);
    query.bindValue(2, trans_prot);
    query.exec();

    query.prepare("DELETE FROM Device WHERE Service_name = ? AND Service_User_uid = ? AND Service_Trans_prot = ?");
    query.bindValue(0, name);
    query.bindValue(1, uid);
    query.bindValue(2, trans_prot);
    query.exec();

    query.prepare("DELETE FROM Service WHERE name = ? AND user_uid = ? and trans_prot = ?");
    query.bindValue(0, name);
    query.bindValue(1, uid);
    query.bindValue(2, trans_prot);
    query.exec();
    // test query
    return true;
}

bool BonjourSQL::insertDevice(QString hostname, int port, QString service_name, QString service_trans_prot, QString record_name) {
    QSqlQuery query = QSqlQuery(db);
    query.prepare("INSERT INTO Record VALUES(?, ?, ?, ?, ?, ?)");
    query.bindValue(0, hostname);
    query.bindValue(1, port);
    query.bindValue(2, service_name);
    query.bindValue(3, uid);
    query.bindValue(4, service_trans_prot);
    query.bindValue(5, record_name);
    query.exec();
    // test qry
    qDebug() << hostname << " " << port << " " << service_name << " " << service_trans_prot << " " << record_name;
    qDebug() << "Insert device error : " << db.lastError().text();
    return true;
}

QString BonjourSQL::fetchXmlRpc() {
    qDebug() << "sql threadid " << QThread::currentThreadId();
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    QString sqlString;
    bool first = true;
    bool skipOr = false;
    foreach(QHostAddress addr, list) {
        QString stringAddr = addr.toString();
        // crop off the interface names: "%en1" for example
        int percentIndex = stringAddr.indexOf("%");
        if (percentIndex > -1)
            stringAddr.truncate(percentIndex);

        // remove local addresses, only used global ones
        if (stringAddr.startsWith("::") || stringAddr.startsWith("fe80")) { // TODO more precise
            skipOr = true;
            continue;
        } else skipOr = false;

        if (!first && !skipOr)
            sqlString = sqlString % " OR ";
        sqlString = sqlString % "\"" % stringAddr % "\"";
        first = false;
    }
    qDebug() << sqlString;
    QSqlQuery query = QSqlQuery(db);
    query.prepare("SELECT MIN(id) as id, req, ipv6 FROM XMLRPC WHERE ipv6 = " % sqlString);
    query.exec();
    //qDebug() << "error" << query.lastError();
    if (query.next()) {
        QString ipv6 = query.value(2).toString();
        QString id = query.value(0).toString();
        QString xmlrpcReq = query.value(1).toString();
        qDebug() << xmlrpcReq;

        query.prepare("DELETE FROM XMLRPC WHERE id = ? AND ipv6 = ?");
        query.bindValue(0, id);
        query.bindValue(1, ipv6);
        query.exec();

        return xmlrpcReq;
    }
    return NULL;
}


QList< User* > BonjourSQL::getFriends() {
    QSqlQuery query = QSqlQuery(db);
    query.prepare("SELECT uid, ipv6, certificate, privateKey FROM User WHERE uid IN (SELECT id as "
                  "uid FROM Authorized_user WHERE Record_Service_User_uid = ?)");
    query.bindValue(0, uid);
    query.exec();
    //qDebug() << "error" << query.lastError();
    QList < User* > list;
    while (query.next()) {
        QString* uid = new QString(query.value(0).toString());
        QString* ipv6 = new QString(query.value(1).toString());
        QSslCertificate* cert = new QSslCertificate(query.value(2).toByteArray(), QSsl::Pem);
        QSslKey* key = new QSslKey(query.value(3).toByteArray(), QSsl::Rsa, QSsl::Pem);

        list.append(new User(uid, ipv6, cert, key));
    }
    return list;
}















