#ifndef Q_HTTP_REQUEST_H
#define Q_HTTP_REQUEST_H

#include <QtCore>
#include <QtXml>
#include <QtNetwork>
#include "maiaFault.h"

#if QT_VERSION >= 0x050000
class QHttpRequestHeader
{
public:
    explicit QHttpRequestHeader(QString headerString);
    virtual ~QHttpRequestHeader() {}

    bool isValid();
    QString method();
    uint contentLength() const;

private:
    QString mHeaderString;
    QString mMethod;
    QMap<QString, QString> mHeaders;
};

class QHttpResponseHeader
{
public:
    explicit QHttpResponseHeader(int code, QString text);
    virtual ~QHttpResponseHeader() {}
    void setValue(const QString &key, const QString &value);
    virtual QString toString() const;

private:
    int mCode;
    QString mText;
    QMap<QString, QString> mHeaders;
};
#endif

#endif
