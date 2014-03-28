#ifndef CONFIG_H
#define CONFIG_H
/**
 * @brief The Config class is an uninstantiable class that contains the static configuration.
 */
#include <QString>
class Config
{
private:
    Config();
public:
    QString webAppHostname = "2001:470:7973::5e96:9dff:fe8a:8447";
};

#endif // CONFIG_H
