#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>


class AudioPlayer
{
public:

    static void begin();

    static void start();

    static void stop();

    static bool isPlaying();

private:

    static volatile bool playing;
};


#endif