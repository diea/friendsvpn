#ifndef CONFIG_H
#define CONFIG_H
/**
 * @brief Configuration macros.
 */
#define TEST

#ifdef TEST
#define WEBAPPHOST "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447"
#define DBHOST "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447"
#endif

#ifdef PRODUCTION
#define WEBAPPHOST "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447"
#define DBHOST "fd3b:e180:cbaa:1:5e96:9dff:fe8a:8447"
#endif

#define DBNAME "friendsvpn"
#define DBUSER "diea"
#define DBPASS ""

#define DATAPLANEPORT 61324
#define CONTROLPLANELISTENPORT 61323


#ifdef __APPLE__
#define HELPERPATH "../../../../friendsvpn/helpers/"
#elif __GNUC__
#define HELPERPATH "./helpers/"
#endif

#endif // CONFIG_H
