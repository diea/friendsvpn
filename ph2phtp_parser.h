#ifndef PH2PHTP_PARSER_H
#define PH2PHTP_PARSER_H

#include <QObject>

/**
 * @brief The PH2PHTP_Parser class implements the text based protocol "Proxy host to proxy host
 * transport protocol".
 */
class PH2PHTP_Parser : public QObject
{
    Q_OBJECT
private:
    explicit PH2PHTP_Parser(QObject *parent = 0);

public:
    /**
     * @brief parseControlPlane will parse a control plane message, it be terminated with a line
     * containing only "\r\n".
     * @param buf
     * @return true, false on failure.
     */
    static bool parseControlPlane(const char* buf, QString friendUid);

    //static bool sendData(const char* buf, int len, QString friendUid);
signals:

public slots:

};

#endif // PH2PHTP_PARSER_H
