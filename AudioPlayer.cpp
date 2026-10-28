#include "AudioPlayer.h"

#include "Pins.h"
#include "Config.h"
#include "SoundManager.h"
#include "AlarmManager.h"

#include <Arduino.h>
#include <math.h>
#include <LittleFS.h>

#include "driver/i2s.h"



// =====================================================
// AUDIO SETTINGS
// =====================================================

#define SAMPLE_RATE     AUDIO_SAMPLE_RATE

#define AMPLITUDE       24000

#define CHUNK_SIZE      256

#define TASK_STACK       8192

#define TASK_PRIORITY    5

#define TASK_CORE        1

#define WAV_FORMAT_PCM          0x0001
#define WAV_FORMAT_EXTENSIBLE   0xFFFE



// =====================================================
// WAV INFO
// =====================================================
//
// Filled in by parseWav() after walking the RIFF
// chunks. fmt / data may appear in any order and
// may be preceded by LIST, JUNK, bext ... chunks.
//

struct WavInfo
{
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint16_t bitsPerSample;

    uint32_t dataOffset;
    uint32_t dataSize;
};



// =====================================================
// MELODY
// =====================================================
//
// Twinkle Twinkle Little Star
//
// { frequency_hz, duration_ms }
//

static const int melody[][2] =
{
    {523, 300},
    {523, 300},
    {784, 300},
    {784, 300},

    {880, 300},
    {880, 300},

    {784, 600},

    {698, 300},
    {698, 300},
    {659, 300},
    {659, 300},

    {587, 300},
    {587, 300},

    {523, 600}
};

static const int numNotes =
    sizeof(melody) / sizeof(melody[0]);



// =====================================================
// STATIC VARIABLES
// =====================================================

volatile bool AudioPlayer::playing = false;

static TaskHandle_t audioTaskHandle = NULL;

// Sound chosen at start() time, read by the task

static char currentSound[sizeof(AlarmSettings::soundName)];

// 0 = play until stop(), otherwise millis() deadline

static volatile unsigned long stopAt = 0;



// =====================================================
// FORWARD DECLARATIONS
// =====================================================



static bool keepPlaying();

static bool playTone(
    int frequency,
    int duration_ms
);

static void playBuiltinMelody();

static void playWavFile(
    const String &path
);

static bool parseWav(
    File &file,
    WavInfo &info,
    String &error
);



// =====================================================
// BEGIN
// =====================================================

void AudioPlayer::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING AUDIO PLAYER");
    Serial.println("==============================");


    i2s_config_t config = {};

    config.mode = (i2s_mode_t)(
        I2S_MODE_MASTER |
        I2S_MODE_TX
    );

    config.sample_rate          = SAMPLE_RATE;
    config.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count        = 4;
    config.dma_buf_len          = 256;
    config.use_apll             = false;
    config.tx_desc_auto_clear   = true;


    i2s_pin_config_t pins = {};

    pins.mck_io_num   = I2S_PIN_NO_CHANGE;
    pins.bck_io_num   = I2S_BCLK_PIN;
    pins.ws_io_num    = I2S_LRC_PIN;
    pins.data_out_num = I2S_DOUT_PIN;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;


    if (
        i2s_driver_install(
            I2S_NUM_0,
            &config,
            0,
            NULL
        ) != ESP_OK
    )
    {
        Serial.println("I2S driver install FAILED!");
        return;
    }


    i2s_set_pin(
        I2S_NUM_0,
        &pins
    );


    i2s_zero_dma_buffer(
        I2S_NUM_0
    );


    Serial.println("Audio Ready");
}



// =====================================================
// START
// =====================================================
//
// soundName: "" = the sound saved in the alarm
//            settings, otherwise a specific sound
//            (used by the web "Test" button).
//
// maxDurationMs: 0 = play until stop()
//

void AudioPlayer::start(
    const String &soundName,
    unsigned long maxDurationMs
)
{
    if (playing)
    {
        return;
    }


    // An old task may still be finishing

    if (audioTaskHandle != NULL)
    {
        return;
    }


    String sound = soundName;

    if (sound.length() == 0)
    {
        sound = AlarmManager::getSettings().soundName;
    }

    strncpy(
        currentSound,
        sound.c_str(),
        sizeof(currentSound) - 1
    );

    currentSound[sizeof(currentSound) - 1] = '\0';


    stopAt =
        (maxDurationMs > 0)
            ? millis() + maxDurationMs
            : 0;


    Serial.println();
    Serial.println("==============================");
    Serial.println("AUDIO START");
    Serial.println("==============================");

    Serial.printf(
        "Sound: %s\n",
        currentSound
    );


    playing = true;


    BaseType_t ok = xTaskCreatePinnedToCore(
        audioTask,
        "audioTask",
        TASK_STACK,
        NULL,
        TASK_PRIORITY,
        &audioTaskHandle,
        TASK_CORE
    );

    if (ok != pdPASS)
    {
        Serial.println("Audio task create FAILED!");

        audioTaskHandle = NULL;

        playing = false;
    }
}



// =====================================================
// STOP
// =====================================================

void AudioPlayer::stop()
{
    if (!playing && audioTaskHandle == NULL)
    {
        return;
    }


    Serial.println();
    Serial.println("==============================");
    Serial.println("AUDIO STOP");
    Serial.println("==============================");


    playing = false;


    // Wait for the task to actually exit so two
    // tasks never write to I2S at the same time

    unsigned long waitStart = millis();

    while (
        audioTaskHandle != NULL &&
        millis() - waitStart < 1000
    )
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }


    // Silence the output

    i2s_zero_dma_buffer(
        I2S_NUM_0
    );
}



// =====================================================
// IS PLAYING
// =====================================================

bool AudioPlayer::isPlaying()
{
    return playing;
}



// =====================================================
// IS PREVIEW (time-limited test playback)
// =====================================================

bool AudioPlayer::isPreview()
{
    return playing && stopAt != 0;
}



// =====================================================
// KEEP PLAYING? (called from the audio task)
// =====================================================

static bool keepPlaying()
{
    if (!AudioPlayer::isPlaying())
    {
        return false;
    }

    if (
        stopAt != 0 &&
        (long)(millis() - stopAt) >= 0
    )
    {
        return false;
    }

    return true;
}



// =====================================================
// PLAY TONE
// =====================================================

static bool playTone(
    int frequency,
    int duration_ms
)
{
    int totalSamples =
        (SAMPLE_RATE * duration_ms) / 1000;

    float phaseIncrement =
        2.0f * M_PI * frequency / SAMPLE_RATE;

    float phase = 0.0f;

    // Get current volume (0-100)
    int volume = AlarmManager::getSettings().volume;
    float currentAmp = AMPLITUDE * (volume / 100.0f);

    // Stereo buffer: L,R,L,R...

    int16_t buffer[CHUNK_SIZE * 2];

    int samplesWritten = 0;


    while (
        samplesWritten < totalSamples
    )
    {
        if (!keepPlaying())
        {
            return false;
        }


        int remaining =
            totalSamples - samplesWritten;

        int count =
            (remaining < CHUNK_SIZE)
                ? remaining
                : CHUNK_SIZE;


        for (int i = 0; i < count; i++)
        {
            int16_t sample = (int16_t)(
                sinf(phase) * currentAmp
            );

            // Left + Right (stereo pair)

            buffer[i * 2]     = sample;
            buffer[i * 2 + 1] = sample;

            phase += phaseIncrement;

            if (phase >= 2.0f * M_PI)
            {
                phase -= 2.0f * M_PI;
            }
        }


        size_t bytesWritten = 0;

        i2s_write(
            I2S_NUM_0,
            buffer,
            count * 2 * sizeof(int16_t),
            &bytesWritten,
            portMAX_DELAY
        );

        samplesWritten += count;
    }

    return true;
}



// =====================================================
// PLAY BUILTIN MELODY
// =====================================================

static void playBuiltinMelody()
{
    // A failed WAV may have changed the rate

    i2s_set_sample_rates(
        I2S_NUM_0,
        SAMPLE_RATE
    );


    while (keepPlaying())
    {
        for (int i = 0; i < numNotes; i++)
        {
            bool ok = playTone(
                melody[i][0],
                melody[i][1]
            );

            if (!ok)
            {
                return;
            }
        }


        // Gap between repetitions

        if (keepPlaying())
        {
            vTaskDelay(
                pdMS_TO_TICKS(400)
            );
        }
    }
}



// =====================================================
// PARSE WAV
// =====================================================
//
// Walks every RIFF chunk looking for "fmt " and
// "data". Leaves the file positioned at the start
// of the audio data on success.
//

static bool parseWav(
    File &file,
    WavInfo &info,
    String &error
)
{
    uint8_t riff[12];

    if (
        file.read(riff, 12) != 12 ||
        memcmp(riff, "RIFF", 4) != 0 ||
        memcmp(riff + 8, "WAVE", 4) != 0
    )
    {
        error = "Not a WAV file";
        return false;
    }


    bool foundFmt = false;
    bool foundData = false;

    uint32_t fileSize = file.size();
    uint32_t pos = 12;


    while (pos + 8 <= fileSize)
    {
        file.seek(pos);

        char chunkId[4];
        uint32_t chunkSize;

        if (
            file.read((uint8_t *)chunkId, 4) != 4 ||
            file.read((uint8_t *)&chunkSize, 4) != 4
        )
        {
            break;
        }


        uint32_t body = pos + 8;


        if (memcmp(chunkId, "fmt ", 4) == 0)
        {
            uint8_t fmt[16];

            if (
                chunkSize < 16 ||
                file.read(fmt, 16) != 16
            )
            {
                error = "Bad fmt chunk";
                return false;
            }

            info.audioFormat   = fmt[0]  | (fmt[1]  << 8);
            info.numChannels   = fmt[2]  | (fmt[3]  << 8);
            info.sampleRate    = fmt[4]  | (fmt[5]  << 8) |
                                 (fmt[6] << 16) | ((uint32_t)fmt[7] << 24);
            info.bitsPerSample = fmt[14] | (fmt[15] << 8);

            foundFmt = true;
        }
        else if (memcmp(chunkId, "data", 4) == 0)
        {
            info.dataOffset = body;

            // Some encoders write 0 / 0xFFFFFFFF
            // when streaming; clamp to the file

            if (
                chunkSize == 0 ||
                body + chunkSize > fileSize
            )
            {
                chunkSize = fileSize - body;
            }

            info.dataSize = chunkSize;

            foundData = true;
        }


        if (foundFmt && foundData)
        {
            break;
        }


        // Chunks are padded to an even size

        pos = body + chunkSize + (chunkSize & 1);
    }


    if (!foundFmt)
    {
        error = "No fmt chunk";
        return false;
    }

    if (!foundData)
    {
        error = "No data chunk";
        return false;
    }

    if (
        info.audioFormat != WAV_FORMAT_PCM &&
        info.audioFormat != WAV_FORMAT_EXTENSIBLE
    )
    {
        error = "Not PCM (compressed WAV)";
        return false;
    }

    if (
        info.bitsPerSample != 8 &&
        info.bitsPerSample != 16
    )
    {
        error = "Must be 8 or 16-bit (file is " +
                String(info.bitsPerSample) + "-bit)";
        return false;
    }

    if (
        info.numChannels != 1 &&
        info.numChannels != 2
    )
    {
        error = "Must be mono or stereo";
        return false;
    }

    if (
        info.sampleRate < 8000 ||
        info.sampleRate > 48000
    )
    {
        error = "Sample rate must be 8000-48000 Hz";
        return false;
    }


    file.seek(info.dataOffset);

    return true;
}



// =====================================================
// CHECK WAV FILE (used after upload)
// =====================================================

bool AudioPlayer::checkWavFile(
    const String &path,
    String &error
)
{
    File file = LittleFS.open(path, "r");

    if (!file)
    {
        error = "File not found";
        return false;
    }

    WavInfo info;

    bool ok = parseWav(file, info, error);

    file.close();

    if (ok)
    {
        Serial.printf(
            "WAV OK: %u Hz, %u ch, %u bit\n",
            info.sampleRate,
            info.numChannels,
            info.bitsPerSample
        );
    }

    return ok;
}



// =====================================================
// PLAY WAV FILE
// =====================================================

static void playWavFile(
    const String &path
)
{
    Serial.printf(
        "Playing WAV: %s\n",
        path.c_str()
    );


    // Stereo output buffer: L,R,L,R...

    int16_t out[CHUNK_SIZE * 2];

    // Raw file data (up to 16-bit stereo)

    uint8_t raw[CHUNK_SIZE * 2 * sizeof(int16_t)];


    while (keepPlaying())
    {
        File file = LittleFS.open(path, "r");

        if (!file)
        {
            Serial.println(
                "WAV file not found! "
                "Falling back to builtin."
            );

            playBuiltinMelody();
            return;
        }


        WavInfo info;
        String error;

        if (!parseWav(file, info, error))
        {
            Serial.printf(
                "WAV error: %s. "
                "Falling back to builtin.\n",
                error.c_str()
            );

            file.close();
            playBuiltinMelody();
            return;
        }


        i2s_set_sample_rates(
            I2S_NUM_0,
            info.sampleRate
        );


        int volume = AlarmManager::getSettings().volume;

        bool mono = (info.numChannels == 1);

        bool eightBit = (info.bitsPerSample == 8);

        size_t sampleBytes = eightBit ? 1 : 2;

        size_t frameBytes = info.numChannels * sampleBytes;

        uint32_t remaining = info.dataSize;


        while (
            remaining >= frameBytes &&
            keepPlaying()
        )
        {
            size_t want = CHUNK_SIZE * frameBytes;

            if (want > remaining)
            {
                want = remaining - (remaining % frameBytes);
            }


            size_t bytesRead = file.read(raw, want);

            if (bytesRead < frameBytes)
            {
                break;
            }

            remaining -= bytesRead;


            // Convert to 16-bit stereo with volume

            size_t frames = bytesRead / frameBytes;

            for (size_t i = 0; i < frames; i++)
            {
                for (int ch = 0; ch < 2; ch++)
                {
                    // Mono: same sample on L and R

                    size_t idx =
                        i * info.numChannels +
                        (mono ? 0 : ch);

                    int32_t s;

                    if (eightBit)
                    {
                        // 8-bit WAV is unsigned

                        s = ((int32_t)raw[idx] - 128) << 8;
                    }
                    else
                    {
                        s = (int16_t)(
                            raw[idx * 2] |
                            (raw[idx * 2 + 1] << 8)
                        );
                    }

                    out[i * 2 + ch] =
                        (int16_t)(s * volume / 100);
                }
            }


            size_t bytesWritten = 0;

            i2s_write(
                I2S_NUM_0,
                out,
                frames * 2 * sizeof(int16_t),
                &bytesWritten,
                portMAX_DELAY
            );
        }


        file.close();


        // Small gap before looping

        if (keepPlaying())
        {
            vTaskDelay(
                pdMS_TO_TICKS(200)
            );
        }
    }


    // Restore default sample rate

    i2s_set_sample_rates(
        I2S_NUM_0,
        SAMPLE_RATE
    );
}



// =====================================================
// AUDIO TASK
// =====================================================

void AudioPlayer::audioTask(void *param)
{
    Serial.println("Audio task started");


    String name = currentSound;

    if (SoundManager::isBuiltin(name))
    {
        playBuiltinMelody();
    }
    else
    {
        playWavFile(
            SoundManager::getWavPath(name)
        );
    }


    // Clean up

    i2s_zero_dma_buffer(
        I2S_NUM_0
    );


    Serial.println("Audio task ended");

    stopAt = 0;

    AudioPlayer::playing = false;

    audioTaskHandle = NULL;

    vTaskDelete(NULL);
}
