/*
 * Copyright (c) 2022 Marcel Licence
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie k�nnen es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * ver�ffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es n�tzlich sein wird, jedoch
 * OHNE JEDE GEW�HR,; sogar ohne die implizite
 * Gew�hr der MARKTF�HIGKEIT oder EIGNUNG F�R EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License f�r weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/**
 * @file midi_stream_player.ino
 * @author Marcel Licence
 * @date 22.05.2022
 *
 * @brief This file contains code to access the midi file stream player
 *
 * The midi file stream player will be part of the ML_SynthTools library
 * This file is required to define all file accessing functions which are not part of the library
 * In addition to that they are dependend to the different platform you are using
 */


#ifdef __CDT_PARSER__
#include <cdt.h>
#endif


#ifdef MIDI_STREAM_PLAYER_ENABLED


#ifdef ARDUINO_DAISY_SEED
#include <STM32SD.h>

extern Sd2Card card;
extern SdFatFs fatFs;

#define SD_MMC  SD

#define FST SDClass
#else
#define FST fs::FS
#endif

#define MIDI_FS_LITTLE_FS   0
#define MIDI_FS_SD_MMC  1


#define FORMAT_LITTLEFS_IF_FAILED true


#include <FS.h>
#ifdef ARDUINO_RUNNING_CORE /* tested with arduino esp32 core version 2.0.2 */
#include <LittleFS.h> /* Using library LittleFS at version 2.0.0 from https://github.com/espressif/arduino-esp32 */
#else
#include <LITTLEFS.h> /* Using library LittleFS_esp32 at version 1.0.6 from https://github.com/lorol/LITTLEFS */
#define LittleFS LITTLEFS
#endif
#include <SD_MMC.h>

#include <ml_midi_file_stream.h>


static void listDir(FST &fs, const char *dirname, uint8_t levels);


struct file_access_f mdiCallbacks =
{
    MIDI_open,
    MIDI_read,
    MIDI_write,
    MIDI_close,
    0,
    MIDI_getc,
    MIDI_putc,
    MIDI_tell,
    MIDI_seek,
};

fs::File midiFile;

uint8_t MIDI_open(const char *path, const char *mode)
{
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    {
        Serial.println("LITTLEFS Mount Failed");
        return 0;
    }
    midiFile = LittleFS.open(path);
    if (!midiFile)
    {
        Serial.println("- failed to open file");
        return 0;
    }
    else
    {
        Serial.printf("File opened: %s\n", path);
    }

    return 1;
}

int MIDI_read(void *buf, uint8_t unused, size_t size, struct file_access_f *ff)
{
    File *file = &midiFile;//ff->file;
    for (int i = 0; i < size; i++)
    {
        ((uint8_t *)buf)[i] = file->read();
        ff->file ++;
    }
    return size;
}

int MIDI_write(void *buf, uint8_t unused, size_t size, struct file_access_f *ff)
{
    return 0;
}

void MIDI_close(struct file_access_f *ff)
{
    File *file = &midiFile;//ff->file;
    file->close();
}

char MIDI_getc(struct file_access_f *ff)
{
    File file = midiFile;//ff->file;
    return file.read();
}

char MIDI_putc(char c, struct file_access_f *ff)
{
    return 0;
}

int MIDI_tell(struct file_access_f *ff)
{
#if 0
    File *file = &midiFile;//ff->file;
    return file->size();
#else
    return ff->file - 1;
#endif
}

char MIDI_seek(struct file_access_f *ff, int pos, uint8_t mode)
{
    File *file = &midiFile;//ff->file;
    if (mode == SEEK_SET)
    {
        file->seek(pos, SeekSet);
        ff->file = pos + 1;
    }
    else
    {
        file->seek(pos, SeekCur);
        ff->file += pos;
    }
    //ff->file = file->size() - pos;
    return 0;
}

void MidiStreamPlayer_Init()
{
    MidiStreamPlayer_ListFiles(MIDI_FS_LITTLE_FS);
    MidiStreamPlayer_ListFiles(MIDI_FS_SD_MMC);
}

void MidiStreamPlayer_PlayFile(char *midi_filename)
{
    MidiStreamPlayer_PlayMidiFile_fromLittleFS(midi_filename, 0);
}

static void listDir(FST &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

static bool midiPlaying = false;

void MidiDataCallback(uint8_t *data, uint8_t data_len)
{
    printf("d:");
    for (uint8_t n = 0; n < data_len; n++)
    {
        printf(" %02x", data[n]);
    }
    printf("\n");
}

uint64_t duration = 0;
struct midi_proc_s midi;

void MidiStreamPlayer_NoteOn(uint8_t ch, uint8_t note, uint8_t vel)
{
    Midi_NoteOn(ch, note, vel);
}

void MidiStreamPlayer_NoteOff(uint8_t ch, uint8_t note, uint8_t vel)
{
    Midi_NoteOff(ch, note);
}

void MidiStreamPlayer_ControlChange(uint8_t ch, uint8_t number, uint8_t value)
{
    Midi_ControlChange(ch, number, value);
}

void MidiStreamPlayer_PlayMidiFile_fromLittleFS(char *filename, uint8_t trackToPlay)
{
    Serial.printf("Try to open %s from LittleFS\n", filename);

    midi.raw = MidiDataCallback;
    midi.noteOn = MidiStreamPlayer_NoteOn;
    midi.noteOff = MidiStreamPlayer_NoteOff;
    midi.controlChange = MidiStreamPlayer_ControlChange;
    midi.ff = &mdiCallbacks;

    midi.midi_tempo = (60000000.0 / 100.0);

    midi_file_stream_load(filename, &midi);

    printf("number_of_tracks: %d\n", midi.number_of_tracks);
    printf("file_format: %d\n", midi.file_format);
    printf("division_type_and_resolution: %d\n", interpret_uint16(midi.division_type_and_resolution));

    if (midi.file_format == 1)
    {
        MidiStreamParseTrack(&midi);

        float temp_f = (60000000.0 / midi.midi_tempo);
        printf("midi_tempo: %d\n", midi.midi_tempo);
        printf("tempo: %0.3f\n", temp_f);

        for (uint8_t n = 1; n < trackToPlay; n++)
        {
            MidiStreamSkipTrack(&midi);
        }
    }

    MidiStreamReadTrackPrepare(&midi);

    duration = 0;
    long shortDuration;
    midiPlaying = MidiStreamReadSingleEventTime(&midi, &shortDuration);
    if (midiPlaying)
    {
        Serial.printf("Started midi file playback\n");
    }
    else
    {
        Serial.printf("Couldn't start midi file playback\n");
    }

    duration = shortDuration;
    duration *= SAMPLE_RATE;
    duration *= midi.midi_tempo;
}

void MidiStreamPlayer_Tick(uint32_t ticks)
{
    static uint64_t tickCnt = 0;

    if (midiPlaying)
    {
        uint64_t longTick = ticks;
        longTick *= (uint64_t)interpret_uint16(midi.division_type_and_resolution);
        longTick *= 1000000;
        tickCnt += longTick;

        while ((tickCnt > duration) && midiPlaying)
        {
            tickCnt -= duration;

            midiPlaying &= MidiStreamReadSingleEvent(&midi);

            long shortDuration;
            midiPlaying &= MidiStreamReadSingleEventTime(&midi, &shortDuration);
            duration = shortDuration;
            duration *= SAMPLE_RATE;
            duration *= midi.midi_tempo;
        }
    }
}

void MidiStreamPlayer_ListFiles(uint8_t filesystem)
{
    switch (filesystem)
    {
    case MIDI_FS_LITTLE_FS:
        {
            if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
            {
                Serial.println("LittleFS Mount Failed");
                return;
            }
            listDir(LittleFS, "/", 3);
            break;
        }
    case MIDI_FS_SD_MMC:
        {
            if (!SD_MMC.begin())
            {
                Serial.println("Card Mount Failed");
                return;
            }
            uint8_t cardType = SD_MMC.cardType();

            if (cardType == CARD_NONE)
            {
                Serial.println("No SD_MMC card attached");
                return;
            }

            Serial.print("SD_MMC Card Type: ");
            if (cardType == CARD_MMC)
            {
                Serial.println("MMC");
            }
            else if (cardType == CARD_SD)
            {
                Serial.println("SDSC");
            }
            else if (cardType == CARD_SDHC)
            {
                Serial.println("SDHC");
            }
            else
            {
                Serial.println("UNKNOWN");
            }

            uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
            Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

            listDir(SD_MMC, "/", 0);
        }
        break;
    }
}

#endif

