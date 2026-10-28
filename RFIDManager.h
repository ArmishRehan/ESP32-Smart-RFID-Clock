#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>

class RFIDManager
{
public:

    static void begin();

    static void loop();

    static bool cardDetected();

    static String getLastUID();

    static bool isConnected();

private:

    static bool detected;

    static bool connected;

    static String lastUID;

    static void readCard();
};


#endif