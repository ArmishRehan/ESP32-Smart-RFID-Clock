#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>

#include "Config.h"


class SoundManager
{
public:

    static void begin();

    static void rescan();

    static int getSoundCount();

    static String getSoundName(int index);

    static bool isBuiltin(const String &name);

    static bool isBuiltinSelected();

    static String getSelectedSound();

    static void setSelectedSound(
        const String &name
    );

    static String getWavPath(
        const String &name
    );

    static String sanitizeName(
        const String &filename
    );

    static bool removeSound(
        const String &name
    );

    static bool wavExists(
        const String &name
    );

    static bool hasRoomFor(
        const String &name
    );

private:

    static void scanSounds();

    static String sounds[MAX_WAV_FILES + 1];

    static int soundCount;
};


#endif
