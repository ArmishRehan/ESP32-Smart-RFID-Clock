#include "SoundManager.h"

#include "Config.h"
#include "Storage.h"
#include "Types.h"

#include <LittleFS.h>



// =====================================================
// STATIC VARIABLES
// =====================================================

String SoundManager::sounds[6];

int SoundManager::soundCount = 0;



// =====================================================
// BEGIN
// =====================================================

void SoundManager::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING SOUND MANAGER");
    Serial.println("==============================");


    if (!LittleFS.begin(true))
    {
        Serial.println(
            "LittleFS mount FAILED!"
        );

        return;
    }

    Serial.println("LittleFS mounted");


    // Create sounds directory if needed

    if (!LittleFS.exists(WAV_DIRECTORY))
    {
        LittleFS.mkdir(WAV_DIRECTORY);

        Serial.println(
            "Created /sounds directory"
        );
    }


    scanSounds();


    Serial.printf(
        "Found %d sound(s)\n",
        soundCount
    );
}



// =====================================================
// SCAN SOUNDS
// =====================================================

void SoundManager::scanSounds()
{
    soundCount = 0;


    // First entry is always built-in

    sounds[0] = "builtin";
    soundCount = 1;


    // Scan WAV directory

    File dir = LittleFS.open(WAV_DIRECTORY);

    if (!dir || !dir.isDirectory())
    {
        return;
    }


    File file = dir.openNextFile();

    while (
        file &&
        soundCount < 6
    )
    {
        String name = file.name();

        if (
            name.endsWith(".wav") ||
            name.endsWith(".WAV")
        )
        {
            // Store just the filename

            int lastSlash =
                name.lastIndexOf('/');

            if (lastSlash >= 0)
            {
                name =
                    name.substring(lastSlash + 1);
            }

            sounds[soundCount] = name;
            soundCount++;

            Serial.printf(
                "  Sound: %s\n",
                name.c_str()
            );
        }

        file = dir.openNextFile();
    }

    dir.close();
}



// =====================================================
// GET SOUND COUNT
// =====================================================

int SoundManager::getSoundCount()
{
    return soundCount;
}



// =====================================================
// GET SOUND NAME BY INDEX
// =====================================================

String SoundManager::getSoundName(int index)
{
    if (index < 0 || index >= soundCount)
    {
        return "";
    }

    return sounds[index];
}



// =====================================================
// IS BUILTIN SELECTED?
// =====================================================

bool SoundManager::isBuiltinSelected()
{
    String selected = getSelectedSound();

    return (
        selected == "builtin" ||
        selected == "" ||
        selected == DEFAULT_SOUND_NAME
    );
}



// =====================================================
// GET SELECTED SOUND
// =====================================================

String SoundManager::getSelectedSound()
{
    AlarmSettings settings =
        Storage::loadAlarm();

    return String(settings.soundName);
}



// =====================================================
// SET SELECTED SOUND
// =====================================================

void SoundManager::setSelectedSound(
    const String &name
)
{
    AlarmSettings settings =
        Storage::loadAlarm();

    strncpy(
        settings.soundName,
        name.c_str(),
        sizeof(settings.soundName) - 1
    );

    settings.soundName[
        sizeof(settings.soundName) - 1
    ] = '\0';

    Storage::saveAlarm(settings);

    Serial.printf(
        "Selected sound: %s\n",
        name.c_str()
    );
}



// =====================================================
// GET WAV PATH
// =====================================================

String SoundManager::getWavPath(
    const String &name
)
{
    return String(WAV_DIRECTORY) + "/" + name;
}



// =====================================================
// ADD SOUND
// =====================================================

bool SoundManager::addSound(
    const String &filename
)
{
    if (soundCount >= 6)
    {
        Serial.println(
            "Max sounds reached!"
        );

        return false;
    }


    // Check if already exists

    for (int i = 0; i < soundCount; i++)
    {
        if (sounds[i] == filename)
        {
            return true;
        }
    }


    sounds[soundCount] = filename;
    soundCount++;

    Serial.printf(
        "Added sound: %s\n",
        filename.c_str()
    );

    return true;
}



// =====================================================
// REMOVE SOUND
// =====================================================

bool SoundManager::removeSound(
    const String &name
)
{
    if (name == "builtin")
    {
        return false;
    }


    // Delete from LittleFS

    String path = getWavPath(name);

    if (LittleFS.exists(path))
    {
        LittleFS.remove(path);
    }


    // If this was selected, reset to builtin

    String selected = getSelectedSound();

    if (selected == name)
    {
        setSelectedSound("builtin");
    }


    // Rescan

    scanSounds();

    return true;
}



// =====================================================
// WAV EXISTS?
// =====================================================

bool SoundManager::wavExists(
    const String &name
)
{
    String path = getWavPath(name);

    return LittleFS.exists(path);
}
