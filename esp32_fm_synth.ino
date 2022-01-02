/*
 * Copyright (c) 2021 Marcel Licence
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
 * Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * veröffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es nützlich sein wird, jedoch
 * OHNE JEDE GEWÄHR,; sogar ohne die implizite
 * Gewähr der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/*
 * this file should be opened with arduino, this is the main project file
 *
 * shown in: https://youtu.be/rGTw05GKwvU
 *
 * Author: Marcel Licence
 */


#ifdef __CDT_PARSER__
#include <cdt.h>
#endif

/*
 * global project configuration
 * stays on top of multi-file-compilation
 */
#include "config.h"


#include <Arduino.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <SD_MMC.h>
#include <WiFi.h>

#include <ml_scope.h>
#include <ml_arp.h>
#include <ml_vu_meter.h>


/* to avoid the high click when turning on the microphone */
static float click_supp_gain = 0.0f;

/* this application starts here */
void setup()
{
    // put your setup code here, to run once:
    delay(500);

    Serial.begin(115200);

    Serial.println();

    Serial.printf("Loading data\n");

    Serial.printf("Firmware started successfully\n");

    click_supp_gain = 0.0f;

#ifdef BLINK_LED_PIN
    Blink_Setup();
#endif

#if 0
    Status_Setup();
#endif

#ifdef ESP32_AUDIO_KIT
#ifdef ES8388_ENABLED
    ES8388_Setup();
#else
    ac101_setup();
#endif
#endif


    setup_i2s();
#ifdef ESP32_AUDIO_KIT
    button_setup();
#endif
    Sine_Init();

    Reverb_Setup();

    Delay_Init();

    /*
     * setup midi module / rx port
     */
    Midi_Setup();


#if 0
    setup_wifi();
#else
    WiFi.mode(WIFI_OFF);
#endif

#ifndef ESP8266
    btStop();
    // esp_wifi_deinit();
#endif

    FmSynth_Init();

    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

    /* PSRAM will be fully used by the looper */
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    Core0TaskInit();
}

/*
 * Core 0
 */
/* this is used to add a task to core 0 */
TaskHandle_t Core0TaskHnd;

inline
void Core0TaskInit()
{
    /* we need a second task for the terminal output */
    xTaskCreatePinnedToCore(Core0Task, "CoreTask0", 8000, NULL, 999, &Core0TaskHnd, 0);
}

void Core0TaskSetup()
{
    /*
     * init your stuff for core0 here
     */
    Status_Setup();

#ifdef MIDI_VIA_USB_ENABLED
    usb_setup();
    MIDI_setShortMsgHandler(HandleShortMsg);
#endif
}

void Core0TaskLoop()
{
    /*
     * put your loop stuff for core0 here
     */
    Status_Process();
#ifdef MIDI_VIA_USB_ENABLED
    usb_loop();
#endif
}

void Core0Task(void *parameter)
{
    Core0TaskSetup();

    while (true)
    {
        Core0TaskLoop();

        /* this seems necessary to trigger the watchdog */
        delay(1);
        yield();
    }
}

float master_output_gain = 0.15f;

#ifdef ESP32_AUDIO_KIT
/* little enum to make switching more clear */
enum acSource
{
    acSrcLine,
    acSrcMic
};

/* line in is used by default, so it should not be changed here */
enum acSource selSource = acSrcLine;

/* be carefull when calling this function, microphones can cause very bad feedback!!! */
void App_ToggleSource(uint8_t channel, float value)
{
    if (value > 0)
    {
        switch (selSource)
        {
        case acSrcLine:
            click_supp_gain = 0.0f;
            ac101_setSourceMic();
            selSource = acSrcMic;
            // Status_TestMsg("Input: Microphone");
            break;
        case acSrcMic:
            click_supp_gain = 0.0f;
            ac101_setSourceLine();
            selSource = acSrcLine;
            //  Status_TestMsg("Input: LineIn");
            break;
        }
    }
}
#endif

void App_SetOutputLevel(uint8_t not_used, float value)
{
    master_output_gain = value;
}

static uint32_t sync = 0;

void Midi_SyncRecvd()
{
    sync += 1;
}

void Synth_RealTimeMsg(uint8_t msg)
{
#ifndef MIDI_SYNC_MASTER
    switch (msg)
    {
    case 0xfa: /* start */
        Arp_Reset();
        break;
    case 0xf8: /* Timing Clock */
        Midi_SyncRecvd();
        break;
    }
#endif
}

#ifdef MIDI_SYNC_MASTER

#define MIDI_PPQ    24
#define SAMPLES_PER_MIN  (SAMPLE_RATE*60)

static float midi_tempo = 120.0f;

void MidiSyncMasterLoop(void)
{
    static float midiDiv = 0;
    midiDiv += SAMPLE_BUFFER_SIZE;
    if (midiDiv >= (SAMPLES_PER_MIN) / (MIDI_PPQ * midi_tempo))
    {
        midiDiv -= (SAMPLES_PER_MIN) / (MIDI_PPQ * midi_tempo);
        Midi_SyncRecvd();
    }
}

void Synth_SetMidiMasterTempo(uint8_t unused, float val)
{
    midi_tempo = 60.0f + val * (240.0f - 60.0f);
}

#endif

void Synth_SongPosition(uint16_t pos)
{
    Serial.printf("Songpos: %d\n", pos);
    if (pos == 0)
    {
        Arp_Reset();
    }
}

void Synth_SongPosReset(uint8_t unused, float var)
{
    if (var > 0)
    {
        Synth_SongPosition(0);
    }
}

/*
 * this should avoid having a constant offset on our signal
 * I am not sure if that is required, but in case it can avoid early clipping
 */
static float fl_offset = 0.0f;
static float fr_offset = 0.0f;


static float fl_sample[SAMPLE_BUFFER_SIZE];
static float fr_sample[SAMPLE_BUFFER_SIZE];
static float m1_sample[SAMPLE_BUFFER_SIZE];

#ifndef absf
#define absf(a) ((a>=0.0f)?(a):(-a))
#endif

/*
 * the main audio task
 */
inline void audio_task()
{
#ifdef AUDIO_PASS_THROUGH
    memset(fl_sample, 0, sizeof(fl_sample));
    memset(fr_sample, 0, sizeof(fr_sample));
    memset(m1_sample, 0, sizeof(m1_sample));
#endif

#ifdef ESP32_AUDIO_KIT
    i2s_read_stereo_samples_buff(fl_sample, fr_sample, SAMPLE_BUFFER_SIZE);
#endif

#ifndef AUDIO_PASS_THROUGH
    memset(fl_sample, 0, sizeof(fl_sample));
    memset(fr_sample, 0, sizeof(fr_sample));
    memset(m1_sample, 0, sizeof(m1_sample));
#endif

    for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
    {
        /*
         * this avoids the high peak coming over the mic input when switching to it
         */
        fl_sample[n] *= click_supp_gain;
        fr_sample[n] *= click_supp_gain;

        if (click_supp_gain < 1.0f)
        {
            click_supp_gain += 0.00001f;
        }
        else
        {
            click_supp_gain = 1.0f;
        }

        /* make it a bit quieter */
        fl_sample[n] *= 0.5f;
        fr_sample[n] *= 0.5f;

        /*
         * this removes dc from signal
         */
        fl_offset = fl_offset * 0.99 + fl_sample[n] * 0.01;
        fr_offset = fr_offset * 0.99 + fr_sample[n] * 0.01;

        fl_sample[n] -= fl_offset;
        fr_sample[n] -= fr_offset;
    }

    /*
     * main loop core
     */
    FmSynth_Process(m1_sample, SAMPLE_BUFFER_SIZE);

    /*
     * apply master output gain
     */
    for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
    {
        /* apply master_output_gain */
        m1_sample[n] *= master_output_gain;
    }

    /*
     * little simple delay effect
     */
    Delay_Process_Buff(m1_sample, SAMPLE_BUFFER_SIZE);

    /*
     * add also some reverb
     */
    Reverb_Process(m1_sample, SAMPLE_BUFFER_SIZE);

    /*
     * mix mono signal to stereo out
     */
    for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
    {
        m1_sample[n] *= 2.125f;
        fl_sample[n] += m1_sample[n];
        fr_sample[n] += m1_sample[n];
    }

    /* function blocks and returns when sample is put into buffer */
    if (i2s_write_stereo_samples_buff(fl_sample, fr_sample, SAMPLE_BUFFER_SIZE))
    {
        /* nothing for here */
    }

    Status_Process_Sample(SAMPLE_BUFFER_SIZE);
}

/*
 * this function will be called once a second
 * call can be delayed when one operation needs more time (> 1/44100s)
 */
void loop_1Hz(void)
{
#ifdef ESP32_AUDIO_KIT
    button_loop();
#endif
#ifdef BLINK_LED_PIN
    Blink_Process();
#endif
}

/*
 * this is the main loop
 */
void loop()
{
#ifdef ARP_MODULE_ENABLED
    Arp_Process(sync);
    sync = 0;
#endif

    audio_task(); /* audio tasks blocks for one sample -> 1/44100s */

    static uint32_t loop_cnt;

    loop_cnt += SAMPLE_BUFFER_SIZE;
    if (loop_cnt >= SAMPLE_RATE)
    {
        loop_cnt = 0;
        loop_1Hz();
    }

    /*
     * doing midi only 64 times per sample cycle
     */
    Midi_Process();

#ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_ProcessSync();
#endif
    //Console_Process();
}

/*
 * Callbacks
 */
void MidiCtrl_Cb_NoteOn(uint8_t ch, uint8_t note, float vel)
{
    Arp_NoteOn(ch, note, vel);
}

void MidiCtrl_Cb_NoteOff(uint8_t ch, uint8_t note)
{
    Arp_NoteOff(ch, note);
}

void MidiCtrl_Status_ValueChangedIntArr(const char *descr, int value, int index)
{
    Status_ValueChangedIntArr(descr, value, index);
}

void Arp_Cb_NoteOn(uint8_t ch, uint8_t note, float vel)
{
    FmSynth_NoteOn(ch, note, vel);
}

void Arp_Cb_NoteOff(uint8_t ch, uint8_t note)
{
    FmSynth_NoteOff(ch, note);
}

void Arp_Status_ValueChangedInt(const char *msg, int value)
{
    Status_ValueChangedInt(msg, value);
}

void Arp_Status_LogMessage(const char *msg)
{
    Status_LogMessage(msg);
}

void Arp_Status_ValueChangedFloat(const char *msg, float value)
{
    Status_ValueChangedFloat(msg, value);
}

void Arp_Cb_Step(uint8_t step)
{
    /* ignore */
}
