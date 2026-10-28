#include "AlarmManager.h"

#include "Config.h"
#include "Storage.h"
#include "TimeManager.h"


// =====================================================
// STATIC VARIABLES
// =====================================================

AlarmSettings AlarmManager::settings;

bool AlarmManager::alarmActive = false;

bool AlarmManager::alarmTriggeredToday = false;

int AlarmManager::lastTriggeredDay = -1;


// =====================================================
// BEGIN
// =====================================================

void AlarmManager::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING ALARM MANAGER");
    Serial.println("==============================");


    // Load saved settings

    settings =
        Storage::loadAlarm();


    Serial.println();

    Serial.printf(
        "Alarm: %02d:%02d %s\n",

        settings.hour,
        settings.minute,

        settings.pm
            ? "PM"
            : "AM"
    );


    Serial.printf(
        "Alarm enabled: %s\n",

        settings.enabled
            ? "YES"
            : "NO"
    );


    Serial.println(
        "Alarm Manager OK"
    );
}


// =====================================================
// LOOP
// =====================================================

void AlarmManager::loop()
{
    if (!settings.enabled)
    {
        return;
    }


    if (!TimeManager::isSynced())
    {
        return;
    }


    CurrentTime current =
        TimeManager::getCurrentTime();


    // -------------------------------------------------
    // NEW DAY
    // -------------------------------------------------

    if (
        current.day != lastTriggeredDay
    )
    {
        alarmTriggeredToday = false;
    }


    // -------------------------------------------------
    // ALREADY TRIGGERED TODAY
    // -------------------------------------------------

    if (alarmTriggeredToday)
    {
        return;
    }


    // -------------------------------------------------
    // CHECK ALARM TIME
    // -------------------------------------------------

    if (isAlarmTime())
    {
        Serial.println();
        Serial.println("==============================");
        Serial.println("ALARM TIME REACHED!");
        Serial.println("==============================");


        alarmActive = true;

        alarmTriggeredToday = true;

        lastTriggeredDay =
            current.day;
    }
}


// =====================================================
// CHECK ALARM TIME
// =====================================================

bool AlarmManager::isAlarmTime()
{
    CurrentTime current =
        TimeManager::getCurrentTime();


    // Compare 12-hour time

    if (
        current.hour12 != settings.hour
    )
    {
        return false;
    }


    if (
        current.minute != settings.minute
    )
    {
        return false;
    }


    if (
        current.pm != settings.pm
    )
    {
        return false;
    }


    return true;
}


// =====================================================
// CONVERT 12 HOUR -> 24 HOUR
// =====================================================

int AlarmManager::convertTo24Hour(
    int hour12,
    bool pm
)
{
    if (hour12 == 12)
    {
        return pm ? 12 : 0;
    }


    if (pm)
    {
        return hour12 + 12;
    }


    return hour12;
}


// =====================================================
// GET SETTINGS
// =====================================================

AlarmSettings AlarmManager::getSettings()
{
    return settings;
}


// =====================================================
// SET SETTINGS
// =====================================================

void AlarmManager::setSettings(
    const AlarmSettings &newSettings
)
{
    bool timeChanged =
        newSettings.hour != settings.hour ||
        newSettings.minute != settings.minute ||
        newSettings.pm != settings.pm ||
        newSettings.enabled != settings.enabled;


    settings = newSettings;


    // Keep values valid

    if (settings.hour < 1)
    {
        settings.hour = 1;
    }


    if (settings.hour > 12)
    {
        settings.hour = 12;
    }


    if (settings.minute < 0)
    {
        settings.minute = 0;
    }


    if (settings.minute > 59)
    {
        settings.minute = 59;
    }


    if (settings.volume < 0)
    {
        settings.volume = 0;
    }


    if (settings.volume > 100)
    {
        settings.volume = 100;
    }


    // Save immediately

    Storage::saveAlarm(
        settings
    );


    // -------------------------------------------------
    // IMPORTANT
    // -------------------------------------------------
    //
    // If user changes the alarm while the current
    // alarm is active, stop it.
    //
    // If the alarm time changed, reset
    // alarmTriggeredToday so the new alarm time can
    // trigger today. If only volume / sound changed,
    // keep it, otherwise an alarm dismissed a few
    // seconds ago would fire again in the same minute.
    //

    alarmActive = false;

    if (timeChanged)
    {
        alarmTriggeredToday = false;

        CurrentTime current =
            TimeManager::getCurrentTime();

        lastTriggeredDay =
            current.day;
    }


    Serial.println();
    Serial.println("Alarm settings updated!");

    Serial.printf(
        "New alarm: %02d:%02d %s\n",

        settings.hour,
        settings.minute,

        settings.pm
            ? "PM"
            : "AM"
    );

    Serial.printf(
        "Sound: %s\n",
        settings.soundName
    );
}


// =====================================================
// SET SOUND ONLY
// =====================================================
//
// Changes the alarm sound without touching the
// alarm state (used when a sound is deleted).
//

void AlarmManager::setSound(
    const String &soundName
)
{
    strncpy(
        settings.soundName,
        soundName.c_str(),
        sizeof(settings.soundName) - 1
    );

    settings.soundName[
        sizeof(settings.soundName) - 1
    ] = '\0';

    Storage::saveAlarm(
        settings
    );
}


// =====================================================
// IS ALARM ACTIVE?
// =====================================================

bool AlarmManager::isActive()
{
    return alarmActive;
}


// =====================================================
// WAS ALARM TRIGGERED?
// =====================================================

bool AlarmManager::wasTriggered()
{
    return alarmTriggeredToday;
}


// =====================================================
// STOP ALARM
// =====================================================

void AlarmManager::stopAlarm()
{
    if (!alarmActive)
    {
        return;
    }


    Serial.println();
    Serial.println("==============================");
    Serial.println("ALARM STOPPED");
    Serial.println("==============================");


    alarmActive = false;


    // -------------------------------------------------
    // IMPORTANT
    // -------------------------------------------------
    //
    // Mark today's alarm as handled.
    //
    // This prevents:
    //
    // 12:22 -> alarm
    // RFID -> stop
    // 12:22:05 -> alarm again
    //
    // The alarm will NOT trigger again until tomorrow.
    //

    alarmTriggeredToday = true;


    CurrentTime current =
        TimeManager::getCurrentTime();


    lastTriggeredDay =
        current.day;
}


// =====================================================
// RESET FOR NEXT DAY
// =====================================================

void AlarmManager::resetForNextDay()
{
    alarmActive = false;

    alarmTriggeredToday = false;

    lastTriggeredDay = -1;
}