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

#include "user.h"
#include "config.h"

class BonjourSQL : public QObject
{
    Q_OBJECT

    friend class Poller;

private:
    QSqlDatabase db; // database handle
    QString uid; // the user's facebook uid

    void initDB();

public:
    explicit BonjourSQL(QObject *parent = 0);
    ~BonjourSQL();
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
    bool removeService(QString name, QString trans_prot);
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
     * @brief getLocalCert
     * @return the SslCertificate associated to this user uid
     */
    QSslCertificate getLocalCert();
    /**
     * @brief getLocalCert
     * @return the SslCertificate associated to this user uid
     */
    QSslKey getMyKey();

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

signals:

public slots:

};

#endif // BONJOURSQL_H
