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

#define SAMPLE_RATE     16000

#define AMPLITUDE       24000

#define CHUNK_SIZE      256

#define TASK_STACK       4096

#define TASK_PRIORITY    5

#define TASK_CORE        1



// =====================================================
// WAV HEADER
// =====================================================

#pragma pack(push, 1)
struct WavHeader
{
    char     riff[4];
    uint32_t fileSize;
    char     wave[4];
    char     fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};
#pragma pack(pop)



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



// =====================================================
// FORWARD DECLARATIONS
// =====================================================

static void audioTaskFunc(void *param);

static bool playTone(
    int frequency,
    int duration_ms
);

static void playBuiltinMelody();

static void playWavFile(
    const String &path
);

static bool validateWav(
    WavHeader &header
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

    pins.bck_io_num   = I2S_BCLK_PIN;
    pins.ws_io_num    = I2S_LRC_PIN;
    pins.data_out_num = I2S_DOUT_PIN;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;


    i2s_driver_install(
        I2S_NUM_0,
        &config,
        0,
        NULL
    );


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

void AudioPlayer::start()
{
    if (playing)
    {
        return;
    }


    Serial.println();
    Serial.println("==============================");
    Serial.println("AUDIO START");
    Serial.println("==============================");


    playing = true;


    xTaskCreatePinnedToCore(
        audioTaskFunc,
        "audioTask",
        TASK_STACK,
        NULL,
        TASK_PRIORITY,
        &audioTaskHandle,
        TASK_CORE
    );
}



// =====================================================
// STOP
// =====================================================

void AudioPlayer::stop()
{
    if (!playing)
    {
        return;
    }


    Serial.println();
    Serial.println("==============================");
    Serial.println("AUDIO STOP");
    Serial.println("==============================");


    playing = false;


    // Wait for task to finish

    vTaskDelay(pdMS_TO_TICKS(50));


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
        if (!AudioPlayer::isPlaying())
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
    while (AudioPlayer::isPlaying())
    {
        for (int i = 0; i < numNotes; i++)
        {
            if (!AudioPlayer::isPlaying())
            {
                return;
            }

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

        if (AudioPlayer::isPlaying())
        {
            vTaskDelay(
                pdMS_TO_TICKS(400)
            );
        }
    }
}



// =====================================================
// VALIDATE WAV HEADER
// =====================================================

static bool validateWav(
    WavHeader &header
)
{
    if (
        strncmp(header.riff, "RIFF", 4) != 0
    )
    {
        Serial.println("WAV: Not RIFF");
        return false;
    }

    if (
        strncmp(header.wave, "WAVE", 4) != 0
    )
    {
        Serial.println("WAV: Not WAVE");
        return false;
    }

    if (header.audioFormat != 1)
    {
        Serial.println(
            "WAV: Not PCM format"
        );
        return false;
    }

    if (header.bitsPerSample != 16)
    {
        Serial.println(
            "WAV: Not 16-bit"
        );
        return false;
    }

    Serial.printf(
        "WAV: %d Hz, %d ch, %d bit\n",
        header.sampleRate,
        header.numChannels,
        header.bitsPerSample
    );

    return true;
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


    while (AudioPlayer::isPlaying())
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


        // Read and validate header

        WavHeader header;

        if (
            file.read(
                (uint8_t *)&header,
                sizeof(WavHeader)
            ) != sizeof(WavHeader)
        )
        {
            Serial.println(
                "WAV: Header read failed"
            );

            file.close();
            playBuiltinMelody();
            return;
        }


        if (!validateWav(header))
        {
            file.close();
            playBuiltinMelody();
            return;
        }


        // Reconfigure I2S sample rate
        // if WAV differs

        i2s_set_sample_rates(
            I2S_NUM_0,
            header.sampleRate
        );


        // Skip to data chunk
        //
        // Standard WAV: data starts at
        // byte 44. But some WAVs have
        // extra chunks. Search for "data".

        file.seek(12);

        char chunkId[4];
        uint32_t chunkSize;

        bool foundData = false;

        while (file.available() >= 8)
        {
            file.read(
                (uint8_t *)chunkId, 4
            );

            file.read(
                (uint8_t *)&chunkSize, 4
            );

            if (
                strncmp(chunkId, "data", 4) == 0
            )
            {
                foundData = true;
                break;
            }

            // Skip this chunk

            file.seek(
                file.position() + chunkSize
            );
        }


        if (!foundData)
        {
            Serial.println(
                "WAV: No data chunk"
            );

            file.close();
            playBuiltinMelody();
            return;
        }


        // Stream audio data

        uint8_t buffer[1024];
        size_t bytesRead;
        size_t bytesWritten;

        while (
            file.available() &&
            AudioPlayer::isPlaying()
        )
        {
            bytesRead = file.read(
                buffer,
                sizeof(buffer)
            );

            if (bytesRead == 0)
            {
                break;
            }

            i2s_write(
                I2S_NUM_0,
                buffer,
                bytesRead,
                &bytesWritten,
                portMAX_DELAY
            );
        }


        file.close();


        // Small gap before looping

        if (AudioPlayer::isPlaying())
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

static void audioTaskFunc(void *param)
{
    Serial.println("Audio task started");


    if (SoundManager::isBuiltinSelected())
    {
        playBuiltinMelody();
    }
    else
    {
        String name =
            SoundManager::getSelectedSound();

        String path =
            SoundManager::getWavPath(name);

        playWavFile(path);
    }


    // Clean up

    i2s_zero_dma_buffer(
        I2S_NUM_0
    );


    Serial.println("Audio task ended");

    audioTaskHandle = NULL;

    vTaskDelete(NULL);
}