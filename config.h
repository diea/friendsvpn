#ifndef CONFIG_H
#define CONFIG_H
#include <QCoreApplication>
/**
 * @brief Configuration macros. These set various settings based on the environment where this
 * app is run. These can be used to tweak the application for a particular setup.
 */
//#define TEST
#ifdef __APPLE__
//#define QTCREATOR /* used when the application is launched on OSX from the QTCREATOR */
#endif
#define PRODUCTION

#ifdef TEST
#define DBHOST "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447"
#define DBPORT 3306
#define DBUSER "diea"
#define DBPASS ""
#endif

#ifdef PRODUCTION
#define DBHOST "www.vyncke.org"
#define DBPORT 6033
#define DBUSER "plewyllie"
#define DBPASS "kjub76_4"
#endif

#define DBNAME "friendsvpn"

#define DATAPLANEPORT 61324 /* data plane default listen port */
#define CONTROLPLANELISTENPORT 61323 /* control plane default listen port */
/* careful: if the default listen ports are changed, the outgoing queries will happen on those same ports */

#ifdef QTCREATOR
#define HELPERPATH "../../../../friendsvpn/helpers/"
#elif __GNUC__
#define HELPERPATH QCoreApplication::applicationDirPath() + "../helpers/"
#endif

#define PROXYCLIENT_TIMEOUT 10000 /* a ProxyClient times out after * ms */
#define IP_BUFFER_LENGTH 24 /* The number of IPs prepared in advance */
#define BONJOUR_DELAY 30000 /* BONJOUR messages are sent every * ms */
#define TIMEOUT_DELAY 100000 /* A PH2PHTP connection times out after * s */
#define MAX_PACKET_SIZE 65536
#define FVPN_MTU 1400 /* define MTU of 1400 to be safe */
#define IPV6_MIN_MTU 1280 /* ipv6 minimum MTU from RFC 2460 */
#define FRAG_BUFFER_SIZE 5

#ifdef TEST
#define DTLS_ENCRYPT "eNULL:NULL" /* used to debug the dataplane connection, disables the encryption */
#else
#define DTLS_ENCRYPT "DEFAULT" /* enables encryption using "default" algorithms on the DTLS connection */
#endif

#endif // CONFIG_H
