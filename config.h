#ifndef CONFIG_H
#define CONFIG_H
/**
 * @brief Configuration macros.
 */
//#define TEST
#ifdef __APPLE__
//#define QTCREATOR
#endif
#define PRODUCTION

#ifdef TEST
#define DBHOST "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447"
#endif

#ifdef PRODUCTION
#define DBHOST "2001:470:7973::5e96:9dff:fe8a:8447"
#endif

#define DBNAME "friendsvpn"
#define DBUSER "diea"
#define DBPASS ""

#define DATAPLANEPORT 61324
#define CONTROLPLANELISTENPORT 61323


#ifdef QTCREATOR
#define HELPERPATH "../../../../friendsvpn/helpers/"
#elif __GNUC__
#define HELPERPATH "./helpers/"
#endif

#define PROXYCLIENT_TIMEOUT 10000
#define IP_BUFFER_LENGTH 8
#define BONJOUR_DELAY 30000
#define TIMEOUT_DELAY 10000

//#define DTLS_ENCRYPT "eNULL:NULL"
#define DTLS_ENCRYPT "DEFAULT"

#endif // CONFIG_H
