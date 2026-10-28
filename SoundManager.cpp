#include "SoundManager.h"

#include "Config.h"
#include "AlarmManager.h"
#include "Types.h"

#include <LittleFS.h>



// =====================================================
// STATIC VARIABLES
// =====================================================

String SoundManager::sounds[MAX_WAV_FILES + 1];

int SoundManager::soundCount = 0;



// =====================================================
// BEGIN
// =====================================================

void SoundManager::begin()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("STARTING SOUND MANAGER");
    Serial.println("==============================");


    if (!LittleFS.begin(true))
    {
        Serial.println(
            "LittleFS mount FAILED!"
        );

        return;
    }

    Serial.println("LittleFS mounted");

    Serial.printf(
        "LittleFS: %u / %u bytes used\n",
        (unsigned)LittleFS.usedBytes(),
        (unsigned)LittleFS.totalBytes()
    );


    // Create sounds directory if needed

    if (!LittleFS.exists(WAV_DIRECTORY))
    {
        LittleFS.mkdir(WAV_DIRECTORY);

        Serial.println(
            "Created /sounds directory"
        );
    }


    scanSounds();


    Serial.printf(
        "Found %d sound(s)\n",
        soundCount
    );


    // Saved sound was deleted / never existed?

    if (
        !isBuiltinSelected() &&
        !wavExists(getSelectedSound())
    )
    {
        Serial.printf(
            "Selected sound '%s' missing, "
            "using builtin\n",
            getSelectedSound().c_str()
        );

        setSelectedSound(DEFAULT_SOUND_NAME);
    }
}



// =====================================================
// SCAN SOUNDS
// =====================================================

void SoundManager::scanSounds()
{
    // First entry is always built-in

    sounds[0] = DEFAULT_SOUND_NAME;
    soundCount = 1;


    // Scan WAV directory

    File dir = LittleFS.open(WAV_DIRECTORY);

    if (!dir || !dir.isDirectory())
    {
        return;
    }


    File file = dir.openNextFile();

    while (
        file &&
        soundCount < MAX_WAV_FILES + 1
    )
    {
        String name = file.name();

        // Store just the filename

        int lastSlash =
            name.lastIndexOf('/');

        if (lastSlash >= 0)
        {
            name =
                name.substring(lastSlash + 1);
        }

        String lower = name;
        lower.toLowerCase();

        if (
            !file.isDirectory() &&
            lower.endsWith(".wav")
        )
        {
            sounds[soundCount] = name;
            soundCount++;

            Serial.printf(
                "  Sound: %s (%u bytes)\n",
                name.c_str(),
                (unsigned)file.size()
            );
        }

        file.close();
        file = dir.openNextFile();
    }

    dir.close();


    // Files uploaded by older firmware may have
    // names that are too long / contain spaces.
    // Rename them so they can be selected.

    for (int i = 1; i < soundCount; i++)
    {
        String clean = sanitizeName(sounds[i]);

        if (
            clean.length() == 0 ||
            clean == sounds[i] ||
            LittleFS.exists(getWavPath(clean))
        )
        {
            continue;
        }

        if (
            LittleFS.rename(
                getWavPath(sounds[i]),
                getWavPath(clean)
            )
        )
        {
            Serial.printf(
                "  Renamed: %s -> %s\n",
                sounds[i].c_str(),
                clean.c_str()
            );

            sounds[i] = clean;
        }
    }
}



// =====================================================
// RESCAN (after upload)
// =====================================================

void SoundManager::rescan()
{
    scanSounds();
}



// =====================================================
// GET SOUND COUNT
// =====================================================

int SoundManager::getSoundCount()
{
    return soundCount;
}



// =====================================================
// GET SOUND NAME BY INDEX
// =====================================================

String SoundManager::getSoundName(int index)
{
    if (index < 0 || index >= soundCount)
    {
        return "";
    }

    return sounds[index];
}



// =====================================================
// IS BUILTIN?
// =====================================================

bool SoundManager::isBuiltin(const String &name)
{
    return (
        name == "builtin" ||
        name == "" ||
        name == DEFAULT_SOUND_NAME
    );
}



// =====================================================
// IS BUILTIN SELECTED?
// =====================================================

bool SoundManager::isBuiltinSelected()
{
    return isBuiltin(getSelectedSound());
}



// =====================================================
// GET SELECTED SOUND
// =====================================================
//
// Read from AlarmManager (the in-memory copy) so it
// always matches what the web page shows and saves.
//

String SoundManager::getSelectedSound()
{
    return String(
        AlarmManager::getSettings().soundName
    );
}



// =====================================================
// SET SELECTED SOUND
// =====================================================

void SoundManager::setSelectedSound(
    const String &name
)
{
    AlarmManager::setSound(name);

    Serial.printf(
        "Selected sound: %s\n",
        name.c_str()
    );
}



// =====================================================
// GET WAV PATH
// =====================================================

String SoundManager::getWavPath(
    const String &name
)
{
    return String(WAV_DIRECTORY) + "/" + name;
}



// =====================================================
// SANITIZE FILENAME
// =====================================================
//
// Keeps [A-Za-z0-9._-], replaces everything else
// with '_' and limits the length so the name fits
// AlarmSettings::soundName and LittleFS paths.
// Returns "" if the name is unusable.
//

String SoundManager::sanitizeName(
    const String &filename
)
{
    String name = filename;

    // Some browsers send a full path

    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);

    slash = name.lastIndexOf('\\');
    if (slash >= 0) name = name.substring(slash + 1);


    String lower = name;
    lower.toLowerCase();

    if (!lower.endsWith(".wav"))
    {
        return "";
    }


    String base = name.substring(0, name.length() - 4);

    String clean = "";

    for (unsigned int i = 0; i < base.length(); i++)
    {
        char c = base[i];

        if (
            isalnum((unsigned char)c) ||
            c == '-' ||
            c == '_' ||
            c == '.'
        )
        {
            clean += c;
        }
        else
        {
            clean += '_';
        }
    }


    // Room for ".wav" + terminator

    const unsigned int maxBase =
        sizeof(AlarmSettings::soundName) - 5;

    if (clean.length() > maxBase)
    {
        clean = clean.substring(0, maxBase);
    }

    if (clean.length() == 0)
    {
        clean = "sound";
    }


    return clean + ".wav";
}



// =====================================================
// REMOVE SOUND
// =====================================================

bool SoundManager::removeSound(
    const String &name
)
{
    if (isBuiltin(name))
    {
        return false;
    }


    // Delete from LittleFS

    String path = getWavPath(name);

    if (
        LittleFS.exists(path) &&
        !LittleFS.remove(path)
    )
    {
        return false;
    }


    // If this was selected, reset to builtin

    if (getSelectedSound() == name)
    {
        setSelectedSound(DEFAULT_SOUND_NAME);
    }


    // Rescan

    scanSounds();

    return true;
}



// =====================================================
// WAV EXISTS?
// =====================================================

bool SoundManager::wavExists(
    const String &name
)
{
    if (isBuiltin(name))
    {
        return false;
    }

    String path = getWavPath(name);

    return LittleFS.exists(path);
}



// =====================================================
// ROOM FOR ANOTHER UPLOAD?
// =====================================================
//
// Re-uploading an existing name just overwrites it.
//

bool SoundManager::hasRoomFor(
    const String &name
)
{
    for (int i = 1; i < soundCount; i++)
    {
        if (sounds[i] == name)
        {
            return true;
        }
    }

    return soundCount < MAX_WAV_FILES + 1;
}
