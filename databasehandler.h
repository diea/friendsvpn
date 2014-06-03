#ifndef BONJOURSQL_H
#define BONJOURSQL_H

#include <QObject>
#include <QtSql>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>
#include <QNetworkInterface>
#include <QSslCertificate>
#include <QSslKey>

#include "unixsignalhandler.h"
#include "user.h"
#include "config.h"
#include "bonjour/bonjourrecord.h"

class DatabaseHandler : public QObject
{
    Q_OBJECT

    friend class Poller;

private:
    QString uid; // the user's facebook uid

    void initDB();
    QMutex qryMut;
    static DatabaseHandler* instance;
    explicit DatabaseHandler(QObject *parent = 0);
public:
    static DatabaseHandler* getInstance();
    ~DatabaseHandler();
    /**
     * @brief uidOK
     * It checks that uid has been set, and notifies user to refresh facebook
     * page if not set, that will call the AppListener "appIsRunning" function
     * which sets the uid.
     */
    void uidOK();

    /**
     * @brief insertService: insert a given service for this user
     * @param name
     * @param trans_prot the transport protocol (udp/tcp)
     * @return true on success
     */
    bool insertService(QString name, QString trans_prot);
    /**
     * @brief insertDevice: insert a device for a given service
     * @param ip
     * @param port
     * @param service_name
     * @return true on success
     */
    bool insertDevice(QString hostname, int port, QString service_name, QString service_trans_prot
                      , QString record_name);

    /**
     * @brief getFriendsIps
     * @return a list of the users friends
     */
    QList< User* > getFriends();

    /**
     * @brief pushCert uploads the local certificate to the database
     */
    void pushCert(const QSslCertificate& cert);

    /**
     * @brief fetchXmlRpc fetches an XML RPC request for this host in the
     * database. The requests are processed in a FCFS manner.
     * @return QString containing the XMLRPC request
     */
    QString fetchXmlRpc();

    /**
     * @brief getLocalUid
     * @return the user's local UID.
     */
    QString getLocalUid();

    /**
     * @brief getLocalIP
     * @return the user's IP stored in the DB.
     */
    QString getLocalIP();

    /**
     * @brief getUidFromIP
     * @param uid
     * @return the user's UID associated with the IP given as param
     *          the string "" if there was no IP for that user
     */
    QString getUidFromIP(QString ip);

    /**
     * @brief getName
     * @param uid
     * @return the user's firstname and lastname in the format: "Firstname_Lastname"
     */
    QString getName(QString uid);

    /**
     * @brief getRecordsFor will get the bonjour records that will be sent the friend with uid
     * "friendUid". That will be the records that are authorized for this friend.
     * /!\ don't forget to "delete" the bonjour records when not used anymore
     * @param friendUid
     * @return
     */
    QList< BonjourRecord* > getRecordsFor(QString friendUid);
signals:

public slots:

};

#endif // BONJOURSQL_H
