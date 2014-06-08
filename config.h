#ifndef CONFIG_H
#define CONFIG_H
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
#endif

#ifdef PRODUCTION
#define DBHOST "2001:470:7973::5e96:9dff:fe8a:8447"
#endif

#define DBNAME "friendsvpn"
#define DBUSER "diea"
#define DBPASS ""

#define DATAPLANEPORT 61324 /* data plane default listen port */
#define CONTROLPLANELISTENPORT 61323 /* control plane default listen port */
/* careful: if the default listen ports are changed, the outgoing queries will happen on those same ports */

#ifdef QTCREATOR
#define HELPERPATH "../../../../friendsvpn/helpers/"
#elif __GNUC__
#define HELPERPATH "./helpers/"
#endif

#define PROXYCLIENT_TIMEOUT 10000 /* a ProxyClient times out after * ms */
#define IP_BUFFER_LENGTH 8 /* The number of IPs prepared in advance */
#define BONJOUR_DELAY 30000 /* BONJOUR messages are sent every * ms */
#define TIMEOUT_DELAY 10000 /* A PH2PHTP connection times out after * s */
#define MAX_PACKET_SIZE 65536
#define FVPN_MTU 1400 /* define MTU of 1400 to be safe */
#define IPV6_MIN_MTU 1280 /* ipv6 minimum MTU from RFC 2460 */

#define DTLS_ENCRYPT "eNULL:NULL" /* used to debug the dataplane connection, disables the encryption */
//#define DTLS_ENCRYPT "DEFAULT" /* enables encryption using "default" algorithms on the DTLS connection */



/* define "getch" for use on linux with ncurses */
/* code from http://stackoverflow.com/questions/7469139/what-is-equivalent-to-getch-getche-in-linux */
#include <termios.h>
#include <stdio.h>

static struct termios old, neww;

/* Initialize new terminal i/o settings */
void initTermios(int echo)
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  neww = old; /* make new settings same as old settings */
  neww.c_lflag &= ~ICANON; /* disable buffered i/o */
  neww.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &neww); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void)
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo)
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void)
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void)
{
  return getch_(1);
}
#endif // CONFIG_H
