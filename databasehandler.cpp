#include "databasehandler.h"
#include "bonjour/bonjourdiscoverer.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>

DatabaseHandler* DatabaseHandler::instance = NULL;

DatabaseHandler::DatabaseHandler(QObject *parent) :
    QObject(parent)
{
#ifdef TEST_PROD
#ifdef __APPLE__
    uid = "1086104828";
#elif __GNUC__
    uid = "100008078109463";
#endif
#endif

    strServiceURL = QString("http://"); // TODO change to https
    strServiceURL += DBHOST;
    strServiceURL += "/~plewyllie/rest";
}

DatabaseHandler* DatabaseHandler::getInstance() {
    static QMutex mutex;
    mutex.lock();
    if (!instance) {
        instance = new DatabaseHandler();
    }
    mutex.unlock();
    return instance;
}

DatabaseHandler::~DatabaseHandler() {
    qDebug() << "DatabaseHandler destructor called!";
}

void DatabaseHandler::uidOK() {
    while (uid == NULL) {
        qDebug() << "Waiting for facebook ID, make sure the facebook interface is open in a web browser";
        QThread::sleep(1);
    }
}

bool DatabaseHandler::insertService(QString name, QString trans_prot) {
    QJsonArray values; // array used to do the SQL request, used by the REST API
    values.append(name);
    values.append(uid);
    values.append(trans_prot);

    QByteArray jsonStr = QJsonDocument(values).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());

    QNetworkRequest request(QUrl(strServiceURL + "/insertService"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();
    return true;
}

bool DatabaseHandler::insertDevice(QString hostname, int port, QString service_name,
                                   QString service_trans_prot, QString record_name) {
    QJsonArray values; // array used to do the SQL request, used by the REST API
    values.append(hostname);
    values.append(port);
    values.append(service_name);
    values.append(uid);
    values.append(service_trans_prot);
    values.append(record_name);

    QByteArray jsonStr = QJsonDocument(values).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());

    QNetworkRequest request(QUrl(strServiceURL + "/insertDevice"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();
    return true;
}

QString DatabaseHandler::fetchXmlRpc() {
    // make json array and post it
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    QJsonObject jsObj;
    QJsonArray localIps;
    foreach(QHostAddress addr, list) {
        QString stringAddr = addr.toString();
        // crop off the interface names: "%en1" for example
        int percentIndex = stringAddr.indexOf("%");
        if (percentIndex > -1)
            stringAddr.truncate(percentIndex);

        // remove local addresses, only used global ones and ipv4
        if (!stringAddr.contains(":") ||
                stringAddr.startsWith("::") || stringAddr.startsWith("fe80")) {
            continue;
        }

        localIps.append(stringAddr);
    }

    jsObj["ips"] = localIps;
    QByteArray jsonStr = QJsonDocument(localIps).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());

    QNetworkRequest request(QUrl(strServiceURL + "/fetchXmlRpc"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QJsonParseError jerror;
    QJsonDocument responseJdoc = QJsonDocument::fromJson(reply->readAll(), &jerror);
    if (jerror.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << jerror.errorString();
        return QString(); // error parsing JSON
    }
    QJsonObject obj = responseJdoc.object();

    return obj["req"].toString();
}

QList< User* > DatabaseHandler::getFriends() {
    QJsonObject jsObj;
    jsObj["uid"] = uid;
    QByteArray jsonStr = QJsonDocument(jsObj).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());
    QNetworkRequest request(QUrl(strServiceURL + "/getFriends"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QJsonParseError jerror;
    QJsonDocument responseJdoc = QJsonDocument::fromJson(reply->readAll(), &jerror);
    if (jerror.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << jerror.errorString();
        return QList< User* >(); // error parsing JSON
    }
    
    QList < User* > list; // list of users to build
    QJsonArray arr = responseJdoc.array();
    foreach (QJsonValue objtest, arr) {
        QJsonObject user = objtest.toObject();
        QSslCertificate* cert = new QSslCertificate(user["certificate"].toString().toUtf8(), QSsl::Pem);
        list.append(new User(new QString(user["uid"].toString()), new QString(user["ipv6"].toString()),
                cert));
    }

    return list;
}

void DatabaseHandler::pushCert(const QSslCertificate& cert) {
    QJsonObject jsObj;
    jsObj["uid"] = uid;
    jsObj["cert"] = QString(cert.toPem());
    QByteArray jsonStr = QJsonDocument(jsObj).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());
    QNetworkRequest request(QUrl(strServiceURL + "/pushCert"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();
}

QString DatabaseHandler::getLocalUid() {
    return uid;
}

QString DatabaseHandler::getLocalIP() {
    QJsonObject jsObj;
    jsObj["uid"] = uid;
    QByteArray jsonStr = QJsonDocument(jsObj).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());
    QNetworkRequest request(QUrl(strServiceURL + "/getLocalIP"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QJsonParseError jerror;
    QJsonDocument responseJdoc = QJsonDocument::fromJson(reply->readAll(), &jerror);
    if (jerror.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << jerror.errorString() << "it may be that the user has no IP stored in the database!";
        qWarning() << jerror.errorString();
        return QString(); // error parsing JSON
    }

    QJsonObject ipObj = responseJdoc.object();
    return ipObj["ipv6"].toString();
}

QString DatabaseHandler::getName(QString uid) {
    QJsonObject jsObj;
    jsObj["uid"] = uid;
    QByteArray jsonStr = QJsonDocument(jsObj).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());
    QNetworkRequest request(QUrl(strServiceURL + "/getName"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QJsonParseError jerror;
    QJsonDocument responseJdoc = QJsonDocument::fromJson(reply->readAll(), &jerror);
    if (jerror.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << jerror.errorString() << "the user is probably not in the database";
        return QString(); // error parsing JSON
    }

    QJsonObject ipObj = responseJdoc.object();
    return ipObj["firstname"].toString() + "_" + ipObj["lastname"].toString();
}

QString DatabaseHandler::getUidFromIP(QHostAddress IP) {
    if (ipUid.contains(IP)) {
        return ipUid.value(IP);
    }

    QJsonObject jsObj;
    jsObj["ip"] = IP.toString();
    QByteArray jsonStr = QJsonDocument(jsObj).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());
    QNetworkRequest request(QUrl(strServiceURL + "/getUidFromIP"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QJsonParseError jerror;
    QJsonDocument responseJdoc = QJsonDocument::fromJson(reply->readAll(), &jerror);
    if (jerror.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << jerror.errorString() << "the user is probably not in the database";
        return QString(); // error parsing JSON
    }

    QJsonObject ipObj = responseJdoc.object();
    return ipObj["uid"].toString();
}

void DatabaseHandler::addUidForIP(QHostAddress IP, QString uid) {
    ipUid.insert(IP, uid);
}


QList < BonjourRecord* > DatabaseHandler::getRecordsFor(QString friendUid) {
    QJsonObject jsObj;
    jsObj["friendUid"] = friendUid;
    jsObj["uid"] = uid;
    QByteArray jsonStr = QJsonDocument(jsObj).toJson(QJsonDocument::Compact);
    QByteArray postDataSize = QByteArray::number(jsonStr.size());
    QNetworkRequest request(QUrl(strServiceURL + "/getRecordsFor"));
    request.setRawHeader("User-Agent", "FriendsVPN app");
    request.setRawHeader("X-Custom-User-Agent", "FriendsVPN app");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager manager;
    QNetworkReply * reply = manager.post(request, jsonStr);

    QEventLoop loop; // local event loop to wait for the qnetwork reply
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    // Execute the event loop here, now we will wait here until finished() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QJsonParseError jerror;
    QJsonDocument responseJdoc = QJsonDocument::fromJson(reply->readAll(), &jerror);
    if (jerror.error != QJsonParseError::NoError) {
        qWarning() << "Error parsing JSON:" << jerror.errorString();
        return QList< BonjourRecord* >(); // error parsing JSON
    }

    QList < BonjourRecord * > list;
    QList < BonjourRecord * > allActiveRecords = BonjourDiscoverer::getInstance()->getAllActiveRecords();

    QJsonArray arr = responseJdoc.array();
    foreach (QJsonValue objtest, arr) {
        QJsonObject record = objtest.toObject();

        BonjourRecord newRecord(record["Record_name"].toString(),
                                // using the bonjour service name notation
                                record["Record_Service_name"].toString(),
                                "local.");

        foreach (BonjourRecord* rec, allActiveRecords) { // if record found in active record, save it
            if (*rec == newRecord) {
                if (rec->resolved)
                    list.append(rec);
            }
        }
    }

    return list;
}










