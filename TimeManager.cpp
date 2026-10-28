#include "TimeManager.h"

#include "Config.h"

#include <WiFi.h>
#include <time.h>


// =====================================================
// STATIC VARIABLES
// =====================================================

bool TimeManager::synced = false;

unsigned long TimeManager::lastWiFiCheck = 0;


// =====================================================
// BEGIN
// =====================================================

void TimeManager::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING TIME MANAGER");
    Serial.println("==============================");


    connectWiFi();


    // SNTP keeps running in the background and
    // re-syncs by itself whenever Wi-Fi is up

    syncTime();
}


// =====================================================
// CONNECT WIFI
// =====================================================

void TimeManager::connectWiFi()
{
    Serial.println();
    Serial.println("Connecting to Wi-Fi...");


    WiFi.mode(WIFI_STA);

    WiFi.setAutoReconnect(true);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );


    int attempts = 0;


    while (
        WiFi.status() != WL_CONNECTED &&
        attempts < 40
    )
    {
        delay(500);

        Serial.print(".");

        attempts++;
    }


    Serial.println();


    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Wi-Fi connected!");

        Serial.print("IP address: ");

        Serial.println(
            WiFi.localIP()
        );
    }
    else
    {
        Serial.println(
            "Wi-Fi connection FAILED!"
        );
    }
}


// =====================================================
// SYNC TIME
// =====================================================

void TimeManager::syncTime()
{
    Serial.println();

    Serial.println(
        "Synchronizing time..."
    );


    configTime(
        GMT_OFFSET_SEC,
        DAYLIGHT_OFFSET_SEC,
        NTP_SERVER_1,
        NTP_SERVER_2
    );


    struct tm timeInfo;


    int attempts = 0;


    while (
        WiFi.status() == WL_CONNECTED &&
        !getLocalTime(&timeInfo, 500) &&
        attempts < 20
    )
    {
        Serial.print(".");

        attempts++;
    }


    Serial.println();


    if (getLocalTime(&timeInfo, 0))
    {
        synced = true;

        Serial.println(
            "Time synchronized!"
        );


        Serial.printf(
            "Current time: %02d:%02d:%02d\n",

            timeInfo.tm_hour,
            timeInfo.tm_min,
            timeInfo.tm_sec
        );
    }
    else
    {
        synced = false;

        Serial.println(
            "Time synchronization FAILED! "
            "Will keep retrying in background."
        );
    }
}


// =====================================================
// LOOP
// =====================================================

void TimeManager::loop()
{
    unsigned long now = millis();


    // Check Wi-Fi / time every 10 seconds

    if (
        now - lastWiFiCheck < 10000
    )
    {
        return;
    }

    lastWiFiCheck = now;


    // -------------------------------------------------
    // Once the clock has been set it keeps running
    // without Wi-Fi, so a dropped connection must NOT
    // stop the alarm from ringing.
    // -------------------------------------------------

    struct tm timeInfo;

    if (
        !synced &&
        getLocalTime(&timeInfo, 0)
    )
    {
        synced = true;

        Serial.println(
            "Time synchronized!"
        );
    }


    // -------------------------------------------------
    // Reconnect without blocking the main loop
    // (web server, RFID and alarm keep running)
    // -------------------------------------------------

    static bool wasConnected = true;

    bool connected =
        WiFi.status() == WL_CONNECTED;

    // Auto-reconnect handles short drops; only
    // kick it every 30 s so we don't interrupt an
    // attempt that is still in progress

    static unsigned long lastReconnect = 0;

    if (!connected)
    {
        if (wasConnected)
        {
            Serial.println(
                "Wi-Fi disconnected. Reconnecting..."
            );

            lastReconnect = now;
        }
        else if (now - lastReconnect >= 30000)
        {
            lastReconnect = now;

            WiFi.disconnect();

            WiFi.begin(
                WIFI_SSID,
                WIFI_PASSWORD
            );
        }
    }
    else if (!wasConnected)
    {
        Serial.print("Wi-Fi reconnected. IP: ");

        Serial.println(
            WiFi.localIP()
        );
    }

    wasConnected = connected;
}


// =====================================================
// IS TIME SYNCHRONIZED?
// =====================================================

bool TimeManager::isSynced()
{
    return synced;
}


// =====================================================
// GET CURRENT TIME
// =====================================================

CurrentTime TimeManager::getCurrentTime()
{
    CurrentTime current;


    struct tm timeInfo;


    if (
        !getLocalTime(&timeInfo, 10)
    )
    {
        current.hour24 = 0;

        current.hour12 = 12;

        current.minute = 0;

        current.second = 0;

        current.pm = false;

        current.day = 0;

        current.month = 0;

        current.year = 0;


        return current;
    }


    // 24-hour time

    current.hour24 =
        timeInfo.tm_hour;


    // AM / PM

    current.pm =
        (timeInfo.tm_hour >= 12);


    // Convert to 12-hour

    if (
        timeInfo.tm_hour == 0
    )
    {
        current.hour12 = 12;
    }
    else if (
        timeInfo.tm_hour > 12
    )
    {
        current.hour12 =
            timeInfo.tm_hour - 12;
    }
    else
    {
        current.hour12 =
            timeInfo.tm_hour;
    }


    current.minute =
        timeInfo.tm_min;


    current.second =
        timeInfo.tm_sec;


    current.day =
        timeInfo.tm_mday;


    current.month =
        timeInfo.tm_mon + 1;


    current.year =
        timeInfo.tm_year + 1900;


    return current;
}


// =====================================================
// FORMATTED TIME
// =====================================================

String TimeManager::getFormattedTime()
{
    CurrentTime current =
        getCurrentTime();


    char buffer[20];


    snprintf(
        buffer,
        sizeof(buffer),

        "%02d:%02d:%02d %s",

        current.hour12,
        current.minute,
        current.second,

        current.pm
            ? "PM"
            : "AM"
    );


    return String(buffer);
}


// =====================================================
// FORMATTED DATE
// =====================================================

String TimeManager::getFormattedDate()
{
    CurrentTime current =
        getCurrentTime();


    char buffer[20];


    snprintf(
        buffer,
        sizeof(buffer),

        "%02d/%02d/%04d",

        current.day,
        current.month,
        current.year
    );


    return String(buffer);
}