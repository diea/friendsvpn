/*
  simple Qt4 class emulater
*/
#include "qHttpRequest.h"

#if QT_VERSION >= 0x050000
QHttpRequestHeader::QHttpRequestHeader(QString headerString)
{
    this->mHeaderString = headerString;

    QStringList hdrs = headerString.split("\r\n");
    QStringList hdrkv;
    for (int i = 0; i < hdrs.size(); i++) {
        if (hdrs.at(i).trimmed().isEmpty()) break;
        if (i == 0) {
            hdrkv = hdrs.at(i).split(" ");
            this->mMethod = hdrkv.at(0);
        } else {
            hdrkv = hdrs.at(i).split(":");
            this->mHeaders[hdrkv.at(0)] = hdrkv.at(1).trimmed();
        }
    }
}

bool QHttpRequestHeader::isValid()
{
    if (this->mHeaderString.isEmpty()) return false;
    if (this->mMethod != "GET" && this->mMethod != "POST") return false;
    if (this->mHeaders.size() < 2) return false;
    return true;
}

QString QHttpRequestHeader::method()
{
    return this->mMethod;
}

uint QHttpRequestHeader::contentLength() const
{
    uint clen = 0;

    clen = this->mHeaders.value("Content-Length").toUInt();

    return clen;
}

QHttpResponseHeader::QHttpResponseHeader(int code, QString text)
{
    this->mCode = code;
    this->mText = text;
}

void QHttpResponseHeader::setValue(const QString &key, const QString &value)
{
    this->mHeaders[key] = value;
}

QString QHttpResponseHeader::toString() const
{
    QMapIterator<QString, QString> it(this->mHeaders);
    QString hdrstr;

    hdrstr += QString("HTTP/1.1 %1 %2\r\n").arg(this->mCode).arg(this->mText);
    while (it.hasNext()) {
        it.next();
        hdrstr += it.key() + ": " + it.value() + "\r\n";
    }
    hdrstr += "\r\n";

    return hdrstr;
}

#endif