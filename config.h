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
    QString webAppHostname = "localhost";
};

#endif // CONFIG_H
