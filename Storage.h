#ifndef STORAGE_H
#define STORAGE_H

#include "Types.h"


class Storage
{
public:

    static void begin();

    static AlarmSettings loadAlarm();

    static void saveAlarm(
        const AlarmSettings &alarm
    );
};


#endif