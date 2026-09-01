#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>


class SoundManager
{
public:

    static void begin();

    static int getSoundCount();

    static String getSoundName(int index);

    static bool isBuiltinSelected();

    static String getSelectedSound();

    static void setSelectedSound(
        const String &name
    );

    static String getWavPath(
        const String &name
    );

    static bool addSound(
        const String &filename
    );

    static bool removeSound(
        const String &name
    );

    static bool wavExists(
        const String &name
    );

private:

    static void scanSounds();

    static String sounds[6];

    static int soundCount;
};


#endif
