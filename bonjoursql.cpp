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
#ifdef __APPLE__
        /*QMessageBox msgBox;
        msgBox.setText("Make sure the facebook application is open and retry (it may take a few seconds before your ID can be determined)");
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
        }*/
        qDebug() << "Waiting for facebook ID, make sure the facebook interface is open in a web browser";
#elif __GNUC__
        qWarning() << "Your facebook ID was not determined, please retry";
        getchar();
#endif
        QThread::sleep(1);
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
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("INSERT INTO Service VALUES(?, ?, ?)");
    query.bindValue(0, name);
    query.bindValue(1, uid);
    query.bindValue(2, trans_prot);
    query.exec();
    // test query
    qDebug() << "INSERT INTO SERVICE";
    qDebug() << query.lastError().text();
    db.close();
    qryMut.unlock();
    return true;
}

bool BonjourSQL::insertDevice(QString hostname, int port, QString service_name, QString service_trans_prot, QString record_name) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
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
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
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
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT uid, ipv6, certificate FROM User WHERE uid IN (SELECT User_uid as "
                  "uid FROM Authorized_user WHERE Record_Service_User_uid = ?)");
    query.bindValue(0, uid);
    query.exec();

    QList <QString> uidlist;

    QList < User* > list;
    while (query.next()) {
        QString* uid = new QString(query.value(0).toString());
        QString* ipv6 = new QString(query.value(1).toString());
        QSslCertificate* cert = new QSslCertificate(query.value(2).toByteArray(), QSsl::Pem);

        list.append(new User(uid, ipv6, cert));
        uidlist.append(*uid);
    }

    // also get users that shared something to me
    query.prepare("SELECT uid, ipv6, certificate FROM User WHERE uid IN (SELECT Record_Service_User_uid as "
                  "uid FROM Authorized_user WHERE User_uid = ? AND Record_Service_User_uid != ?)");
    query.bindValue(0, uid);
    query.bindValue(1, uid);
    query.exec();

    while (query.next()) {
        QString* uid = new QString(query.value(0).toString());
        QString* ipv6 = new QString(query.value(1).toString());
        QSslCertificate* cert = new QSslCertificate(query.value(2).toByteArray(), QSsl::Pem);
        if (!uidlist.contains(*uid)) {
            list.append(new User(uid, ipv6, cert));
        }
    }

    db.close();
    qryMut.unlock();
    return list;
}

void BonjourSQL::pushCert(const QSslCertificate& cert) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
    QSqlQuery query = QSqlQuery(db);
    query.prepare("UPDATE User SET certificate=? WHERE uid = ?");
    query.bindValue(0, cert.toPem());
    query.bindValue(1, uid);
    query.exec();
    qryMut.unlock();
}

QString BonjourSQL::getLocalUid() {
    return uid;
}

QString BonjourSQL::getLocalIP() {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
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

QString BonjourSQL::getName(QString uid) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT firstname, lastname FROM User WHERE uid = ?");
    query.bindValue(0, uid);
    query.exec();

    if (query.next()) {
        QString firstname = query.value(0).toString();
        QString lastname = query.value(1).toString();
        db.close();
        qryMut.unlock();
        return QString(firstname + "_" + lastname);
    } else {
        // error
        qDebug() << "User" << uid << "not in db";
        db.close();
        qryMut.unlock();
        return QString();
    }
}

QString BonjourSQL::getUidFromIP(QString IP) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
    QSqlQuery query = QSqlQuery(QSqlDatabase::database());
    query.prepare("SELECT uid FROM User WHERE ipv6 = ?");
    query.bindValue(0, IP);
    query.exec();

    if (query.next()) {
        QString user_uid = query.value(0).toString();
        db.close();
        qryMut.unlock();
        return user_uid;
    } else {
        // error
        qDebug() << "No uid for ip " << IP;
        db.close();
        qryMut.unlock();
        return QString();
    }
}


QList < BonjourRecord* > BonjourSQL::getRecordsFor(QString friendUid) {
    qryMut.lock();
    QSqlDatabase db = QSqlDatabase::database();
    while (!db.isOpen()) {
        db.open();
        qDebug() << "Trying to reach DB";
        QThread::sleep(1);
    }
    QList < BonjourRecord * > list;
    QSqlQuery qry(QSqlDatabase::database());

    if (!qry.prepare("SELECT * FROM Authorized_user WHERE User_uid = ? AND Record_Service_User_uid = ?")) {
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
    db.close();
    qryMut.unlock();
    return list;
}










