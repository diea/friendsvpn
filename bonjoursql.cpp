#include "bonjoursql.h"
#include "bonjour/bonjourdiscoverer.h"

BonjourSQL* BonjourSQL::instance = NULL;

BonjourSQL::BonjourSQL(QObject *parent) :
    QObject(parent)
{
#ifdef TEST
#ifdef __APPLE__
    uid = "1086104828"; // TODO REMOVE
#elif __GNUC__
    uid = "100008078109463";
#endif
#endif
    QSqlDatabase db;
    while (!db.open()) {
        initDB();
        db = QSqlDatabase::database();
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

BonjourSQL* BonjourSQL::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (!instance) {
        instance = new BonjourSQL();
    }
    mutex.unlock();
    return instance;
}

BonjourSQL::~BonjourSQL() {
    qDebug() << "BonjourSQL destructor called!";
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
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(DBHOST);
    db.setDatabaseName(DBNAME);
    db.setUserName(DBUSER);
    db.setPassword(DBPASS);
}

bool BonjourSQL::insertService(QString name, QString trans_prot) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("INSERT INTO Service VALUES(?, ?, ?)");
    query.bindValue(0, name);
    query.bindValue(1, uid);
    query.bindValue(2, trans_prot);
    query.exec();
    // test query
    db.close(); qryMut.unlock();
    return true;
}

// TODO test this
bool BonjourSQL::removeService(QString name, QString trans_prot) {
    /*qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
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
    db.close();
    db.close(); qryMut.unlock();
    // test query
    return true;*/
}

bool BonjourSQL::insertDevice(QString hostname, int port, QString service_name, QString service_trans_prot, QString record_name) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("INSERT INTO Record VALUES(?, ?, ?, ?, ?, ?)");
    query.bindValue(0, hostname);
    query.bindValue(1, port);
    query.bindValue(2, service_name);
    query.bindValue(3, uid);
    query.bindValue(4, service_trans_prot);
    query.bindValue(5, record_name);
    query.exec();
    // test qry
    db.close(); qryMut.unlock();
    return true;
}

QString BonjourSQL::fetchXmlRpc() {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    QString sqlString;
    bool first = true;
    bool skipOr = false;
    sqlString = sqlString % "ipv6 = ";
    foreach(QHostAddress addr, list) {
        QString stringAddr = addr.toString();
        // crop off the interface names: "%en1" for example
        int percentIndex = stringAddr.indexOf("%");
        if (percentIndex > -1)
            stringAddr.truncate(percentIndex);

        // remove local addresses, only used global ones and ipv4
        if (!stringAddr.contains(":") ||
                stringAddr.startsWith("::") || stringAddr.startsWith("fe80")) { // TODO more precise
            skipOr = true;
            continue;
        } else skipOr = false;

        if (!first && !skipOr)
            sqlString = sqlString % " OR ipv6 = ";
        sqlString = sqlString % "\"" % stringAddr % "\"";
        first = false;
    }
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT MIN(id) as id, req, ipv6 FROM XMLRPC WHERE " + sqlString);
    query.exec();
    if (query.next()) {
        QString ipv6 = query.value(2).toString();
        QString id = query.value(0).toString();
        QString xmlrpcReq = query.value(1).toString();

        query.prepare("DELETE FROM XMLRPC WHERE id = ? AND ipv6 = ?");
        query.bindValue(0, id);
        query.bindValue(1, ipv6);
        query.exec();
        db.close(); qryMut.unlock();
        return xmlrpcReq;
    }
    db.close(); qryMut.unlock();
    return NULL;
}


QList< User* > BonjourSQL::getFriends() {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT uid, ipv6, certificate, privateKey FROM User WHERE uid IN (SELECT id as "
                  "uid FROM Authorized_user WHERE Record_Service_User_uid = ?)");
    query.bindValue(0, uid);
    query.exec();

    QList < User* > list;
    while (query.next()) {
        QString* uid = new QString(query.value(0).toString());
        QString* ipv6 = new QString(query.value(1).toString());
        QSslCertificate* cert = new QSslCertificate(query.value(2).toByteArray(), QSsl::Pem);
        QSslKey* key = new QSslKey(query.value(3).toByteArray(), QSsl::Rsa, QSsl::Pem);

        list.append(new User(uid, ipv6, cert, key));
    }
    db.close(); qryMut.unlock();
    return list;
}

QSslCertificate BonjourSQL::getLocalCert() {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT certificate FROM User WHERE uid = ?");
    query.bindValue(0, uid);
    query.exec();

    if (query.next()) {
        QSslCertificate cert(query.value(0).toByteArray(), QSsl::Pem);
        db.close(); qryMut.unlock();
        return cert;
    } else {
        // error
        qDebug() << "No certificate for user " << uid;
        db.close(); qryMut.unlock();
        return QSslCertificate(NULL);
    }
}

QSslKey BonjourSQL::getMyKey() {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT privateKey FROM User WHERE uid = ?");
    query.bindValue(0, uid);
    query.exec();

    if (query.next()) {
        QSslKey key(query.value(0).toByteArray(), QSsl::Rsa, QSsl::Pem);
        db.close(); qryMut.unlock();
        return key;
    } else {
        // error
        qDebug() << "No certificate for user " << uid;
        db.close(); qryMut.unlock();
        return QSslKey(NULL);
    }
}

QString BonjourSQL::getLocalUid() {
    return uid;
}

QString BonjourSQL::getLocalIP() {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT ipv6 FROM User WHERE uid = ?");
    query.bindValue(0, uid);
    query.exec();

    if (query.next()) {
        QString ipv6 = query.value(0).toString();
        db.close(); qryMut.unlock();
        return ipv6;
    } else {
        // error
        qDebug() << "No ipv6 for user " << uid;
        db.close(); qryMut.unlock();
        return "::1"; // TODO handle this case
    }
}

QString BonjourSQL::getUidFromIP(QString IP) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT uid FROM User WHERE ipv6 = ?");
    query.bindValue(0, IP);
    query.exec();

    if (query.next()) {
        QString user_uid = query.value(0).toString();
        db.close(); qryMut.unlock();
        return user_uid;
    } else {
        // error
        qDebug() << "No uid for ip " << IP;
        db.close(); qryMut.unlock();
        return "NULL";
    }
}


QList < BonjourRecord* > BonjourSQL::getRecordsFor(QString friendUid) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    QList < BonjourRecord * > list;
    QSqlQuery qry(QSqlDatabase::database());

    if (!qry.prepare("SELECT * FROM Authorized_user WHERE id = ? AND Record_Service_User_uid = ?")) {
        qDebug() << "ERROR SQL " << qry.lastError();
        db.close(); qryMut.unlock();
        return list;
    }

    qry.bindValue(0, friendUid);
    qry.bindValue(1, uid);
    qry.exec();

    QList < BonjourRecord * > allActiveRecords = BonjourDiscoverer::getInstance()->getAllActiveRecords();

    while (qry.next()) {
        // used for comparison
        BonjourRecord newRecord(qry.value("Record_name").toString(),
                                // using the bonjour service name notation
                                qry.value("Record_Service_name").toString() + "._" + qry.value("Record_Service_Trans_prot").toString() + ".",
                                "local."); // TODO Do some research here, should store in DB ?

        foreach (BonjourRecord* rec, allActiveRecords) { // if record found in active record, save it
            if (*rec == newRecord) {
                if (rec->resolved)
                    list.append(rec);
            }
        }
    }
    db.close(); qryMut.unlock();
    return list;
}










