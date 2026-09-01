#ifndef TYPES_H
#define TYPES_H


// =====================================================
// ALARM SETTINGS
// =====================================================

struct AlarmSettings
{
    int hour;

    int minute;

    bool pm;

    bool enabled;

    int volume;

    char soundName[32];
};


// =====================================================
// DEVICE STATUS
// =====================================================

struct DeviceStatus
{
    bool wifiConnected;

    bool timeSynced;

    bool rfidConnected;

    bool alarmActive;

    bool alarmTriggered;
};


// =====================================================
// CURRENT TIME
// =====================================================

struct CurrentTime
{
    int hour24;

    int hour12;

    int minute;

    int second;

    bool pm;

    int day;

    int month;

    int year;
};


#endif