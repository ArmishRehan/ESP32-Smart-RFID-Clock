#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>

#include "Types.h"


class AlarmManager
{
public:

    static void begin();

    static void loop();

    static AlarmSettings getSettings();

    static void setSettings(
        const AlarmSettings &settings
    );

    static bool isActive();

    static bool wasTriggered();

    static void stopAlarm();

    static void resetForNextDay();


private:

    static AlarmSettings settings;

    static bool alarmActive;

    static bool alarmTriggeredToday;

    static int lastTriggeredDay;

    static bool isAlarmTime();

    static int convertTo24Hour(
        int hour12,
        bool pm
    );
};


#endif