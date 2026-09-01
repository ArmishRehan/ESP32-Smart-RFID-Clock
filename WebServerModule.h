#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <Arduino.h>


class WebServerModule
{
public:

    static void begin();

private:

    static void setupRoutes();
};


#endif
