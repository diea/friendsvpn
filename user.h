#ifndef USER_H
#define USER_H

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>

/**
 * @brief The User class represents a user of the system.
 */
class User : public QObject
{
    Q_OBJECT
public:
    const QString* uid;
    const QString* ipv6;
    const QSslCertificate* cert;

    explicit User(QString* uid, QString* ipv6, QSslCertificate* cert, QObject *parent = 0);
    ~User();
signals:

public slots:

};

#endif // USER_H
