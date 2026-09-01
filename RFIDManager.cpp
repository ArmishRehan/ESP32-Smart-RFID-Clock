#include "RFIDManager.h"

#include "Pins.h"

#include <SPI.h>
#include <MFRC522.h>


// =====================================================
// RFID OBJECT
// =====================================================

MFRC522 rfid(
    RFID_SS_PIN,
    RFID_RST_PIN
);


// =====================================================
// STATIC VARIABLES
// =====================================================

bool RFIDManager::detected = false;

String RFIDManager::lastUID = "";


// =====================================================
// BEGIN
// =====================================================

void RFIDManager::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING RFID MANAGER");
    Serial.println("==============================");


    SPI.begin(
        RFID_SCK_PIN,
        RFID_MISO_PIN,
        RFID_MOSI_PIN,
        RFID_SS_PIN
    );


    rfid.PCD_Init();

    delay(100);


    byte version =
        rfid.PCD_ReadRegister(
            MFRC522::VersionReg
        );


    if (
        version == 0x00 ||
        version == 0xFF
    )
    {
        Serial.println(
            "RC522 NOT FOUND!"
        );
    }
    else
    {
        Serial.print(
            "RC522 OK - Version: 0x"
        );

        Serial.println(
            version,
            HEX
        );
    }
}


// =====================================================
// LOOP
// =====================================================

void RFIDManager::loop()
{
    detected = false;


    if (
        !rfid.PICC_IsNewCardPresent()
    )
    {
        return;
    }


    if (
        !rfid.PICC_ReadCardSerial()
    )
    {
        return;
    }


    readCard();


    rfid.PICC_HaltA();

    rfid.PCD_StopCrypto1();
}


// =====================================================
// READ CARD
// =====================================================

void RFIDManager::readCard()
{
    lastUID = "";


    Serial.println();
    Serial.println("==============================");
    Serial.println("RFID CARD DETECTED!");
    Serial.println("==============================");


    Serial.print("UID: ");


    for (
        byte i = 0;
        i < rfid.uid.size;
        i++
    )
    {
        if (
            rfid.uid.uidByte[i] < 0x10
        )
        {
            Serial.print("0");
        }


        Serial.print(
            rfid.uid.uidByte[i],
            HEX
        );


        Serial.print(" ");


        // Build UID string

        if (i > 0)
        {
            lastUID += ":";
        }


        if (
            rfid.uid.uidByte[i] < 0x10
        )
        {
            lastUID += "0";
        }


        lastUID += String(
            rfid.uid.uidByte[i],
            HEX
        );
    }


    lastUID.toUpperCase();


    Serial.println();

    Serial.print("UID string: ");

    Serial.println(lastUID);


    detected = true;
}


// =====================================================
// CARD DETECTED
// =====================================================

bool RFIDManager::cardDetected()
{
    return detected;
}


// =====================================================
// GET UID
// =====================================================

String RFIDManager::getLastUID()
{
    return lastUID;
}