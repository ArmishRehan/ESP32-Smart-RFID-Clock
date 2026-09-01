#ifndef CONFIG_H
#define CONFIG_H


// =====================================================
// DEVICE
// =====================================================

#define DEVICE_NAME "Smart RFID Alarm Clock"

#define FIRMWARE_VERSION "2.0"


// =====================================================
// WIFI
// =====================================================

// Wi-Fi credentials are now stored in env.h which is ignored by git
#include "env.h"


// =====================================================
// TIME
// =====================================================

// Pakistan Standard Time
// UTC +5

#define GMT_OFFSET_SEC (5 * 3600)

#define DAYLIGHT_OFFSET_SEC 0


// NTP servers

#define NTP_SERVER_1 "pool.ntp.org"

#define NTP_SERVER_2 "time.nist.gov"


// =====================================================
// WEB SERVER
// =====================================================

#define WEB_SERVER_PORT 80


// =====================================================
// AUDIO
// =====================================================

// WAV sample rate

#define AUDIO_SAMPLE_RATE 16000

// 16-bit audio

#define AUDIO_BITS 16


// =====================================================
// DEFAULT ALARM
// =====================================================

// Default alarm time:
//
// 07:30 AM

#define DEFAULT_ALARM_HOUR 7

#define DEFAULT_ALARM_MINUTE 30

#define DEFAULT_ALARM_PM false

#define DEFAULT_ALARM_ENABLED true


// =====================================================
// DEFAULT VOLUME
// =====================================================

// Software volume.
//
// 100 = maximum.
//
// Actual loudness also depends on:
// - MAX98357A supply voltage
// - speaker impedance
// - speaker efficiency

#define DEFAULT_VOLUME 100


// =====================================================
// SOUND / WAV
// =====================================================

#define DEFAULT_SOUND_NAME "builtin"

#define MAX_WAV_FILES 5

#define MAX_WAV_FILE_SIZE (500 * 1024)

// Directory in LittleFS for WAV files

#define WAV_DIRECTORY "/sounds"


#endif