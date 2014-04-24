#ifndef PRIVILEGEESCALATOR_H
#define PRIVILEGEESCALATOR_H

#include <QObject>
#include <Security/Authorization.h>

class PrivilegeEscalator : public QObject
{
    Q_OBJECT
public:
    explicit PrivilegeEscalator(QObject *parent = 0);
    void macEscalate();
signals:

public slots:

};

#endif // PRIVILEGEESCALATOR_H
