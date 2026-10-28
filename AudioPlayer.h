#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>


class AudioPlayer
{
public:

    static void begin();

    static void start(
        const String &soundName = "",
        unsigned long maxDurationMs = 0
    );

    static void stop();

    static bool isPlaying();

    static bool isPreview();

    static bool checkWavFile(
        const String &path,
        String &error
    );

private:

    static volatile bool playing;

    static void audioTask(void *param);
};


#endif
