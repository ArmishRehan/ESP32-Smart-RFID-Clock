#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include "Types.h"


class TimeManager
{
public:

    static void begin();

    static void loop();

    static bool isSynced();

    static CurrentTime getCurrentTime();

    static String getFormattedTime();

    static String getFormattedDate();


private:

    static bool synced;

    static unsigned long lastWiFiCheck;

    static void connectWiFi();

    static void syncTime();
};


#endif