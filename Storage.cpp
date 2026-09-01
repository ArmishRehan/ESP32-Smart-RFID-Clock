#include "Storage.h"
#include "Config.h"

#include <Preferences.h>

Preferences preferences;


// =====================================================
// START STORAGE
// =====================================================

void Storage::begin()
{
    Serial.println();
    Serial.println("Starting Storage...");

    preferences.begin(
        "alarm",
        false
    );

    Serial.println("Storage OK");
}


// =====================================================
// LOAD ALARM
// =====================================================

AlarmSettings Storage::loadAlarm()
{
    AlarmSettings alarm;


    alarm.hour =
        preferences.getInt(
            "hour",
            DEFAULT_ALARM_HOUR
        );


    alarm.minute =
        preferences.getInt(
            "minute",
            DEFAULT_ALARM_MINUTE
        );


    alarm.pm =
        preferences.getBool(
            "pm",
            DEFAULT_ALARM_PM
        );


    alarm.enabled =
        preferences.getBool(
            "enabled",
            DEFAULT_ALARM_ENABLED
        );


    alarm.volume =
        preferences.getInt(
            "volume",
            DEFAULT_VOLUME
        );


    String snd =
        preferences.getString(
            "sound",
            DEFAULT_SOUND_NAME
        );

    strncpy(
        alarm.soundName,
        snd.c_str(),
        sizeof(alarm.soundName) - 1
    );

    alarm.soundName[
        sizeof(alarm.soundName) - 1
    ] = '\0';


    Serial.println();
    Serial.println("Loaded alarm settings");

    Serial.printf(
        "Hour: %d\n",
        alarm.hour
    );

    Serial.printf(
        "Minute: %d\n",
        alarm.minute
    );

    Serial.printf(
        "AM/PM: %s\n",
        alarm.pm ? "PM" : "AM"
    );

    Serial.printf(
        "Enabled: %s\n",
        alarm.enabled ? "YES" : "NO"
    );

    Serial.printf(
        "Volume: %d\n",
        alarm.volume
    );

    Serial.printf(
        "Sound: %s\n",
        alarm.soundName
    );


    return alarm;
}


// =====================================================
// SAVE ALARM
// =====================================================

void Storage::saveAlarm(
    const AlarmSettings &alarm
)
{
    preferences.putInt(
        "hour",
        alarm.hour
    );


    preferences.putInt(
        "minute",
        alarm.minute
    );


    preferences.putBool(
        "pm",
        alarm.pm
    );


    preferences.putBool(
        "enabled",
        alarm.enabled
    );


    preferences.putInt(
        "volume",
        alarm.volume
    );


    preferences.putString(
        "sound",
        alarm.soundName
    );


    Serial.println();
    Serial.println("Alarm settings saved!");
}