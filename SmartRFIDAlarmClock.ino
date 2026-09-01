#include "Config.h"
#include "Pins.h"
#include "Types.h"

#include "Storage.h"
#include "TimeManager.h"
#include "AlarmManager.h"
#include "RFIDManager.h"
#include "AudioPlayer.h"
#include "SoundManager.h"
#include "WebServerModule.h"

#include <WebServer.h>

extern WebServer server;


void setup()
{
    Serial.begin(115200);

    delay(1000);


    Serial.println();
    Serial.println("==============================");
    Serial.println("SMART RFID ALARM CLOCK V2");
    Serial.println("==============================");


    // =================================================
    // STORAGE
    // =================================================

    Storage::begin();


    // =================================================
    // TIME (also connects WiFi)
    // =================================================

    TimeManager::begin();


    // =================================================
    // ALARM
    // =================================================

    AlarmManager::begin();


    // =================================================
    // RFID
    // =================================================

    RFIDManager::begin();


    // =================================================
    // AUDIO
    // =================================================

    AudioPlayer::begin();


    // =================================================
    // SOUNDS (LittleFS)
    // =================================================

    SoundManager::begin();


    // =================================================
    // WEB SERVER
    // =================================================

    WebServerModule::begin();


    Serial.println();
    Serial.println("==============================");
    Serial.println("SYSTEM READY");
    Serial.println("==============================");
}


void loop()
{
    // -------------------------------------------------
    // Wi-Fi / time
    // -------------------------------------------------

    TimeManager::loop();


    // -------------------------------------------------
    // Web server
    // -------------------------------------------------

    server.handleClient();


    // -------------------------------------------------
    // Alarm checking
    // -------------------------------------------------

    AlarmManager::loop();


    // -------------------------------------------------
    // RFID
    // -------------------------------------------------

    RFIDManager::loop();


    // -------------------------------------------------
    // RFID stops alarm
    // -------------------------------------------------

    if (
        RFIDManager::cardDetected() &&
        AlarmManager::isActive()
    )
    {
        Serial.println();
        Serial.println("==============================");
        Serial.println("RFID STOP COMMAND");
        Serial.println("==============================");


        // Stop alarm logic

        AlarmManager::stopAlarm();


        // Stop audio immediately

        AudioPlayer::stop();
    }


    // -------------------------------------------------
    // Start audio when alarm becomes active
    // -------------------------------------------------

    if (
        AlarmManager::isActive()
    )
    {
        if (
            !AudioPlayer::isPlaying()
        )
        {
            AudioPlayer::start();
        }
    }


    delay(10);
}