/*
 * this file should be opened with arduino, this is the main project file
 *
 * shown in: https://youtu.be/rGTw05GKwvU
 *
 * Author: Marcel Licence
 */

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

/* this is used to add a task to core 0 */
TaskHandle_t  Core0TaskHnd ;

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

    Blink_Setup();

#if 0
    Status_Setup();
#endif
#ifdef ESP32_AUDIO_KIT
    ac101_setup();
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

    btStop();
    // esp_wifi_deinit();

#if 0
    Delay_Init();
    Delay_Reset();
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

    /* we need a second task for the terminal output */
    xTaskCreatePinnedToCore(Core0Task, "Core0Task", 8000, NULL, 999, &Core0TaskHnd, 0);
}

void Core0TaskSetup()
{
    /*
     * init your stuff for core0 here
     */
    Status_Setup();
}

void Core0TaskLoop()
{
    /*
     * put your loop stuff for core0 here
     */
    Status_Process();
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

#include "AC101.h" /* only compatible with forked repo: https://github.com/marcel-licence/AC101 */
extern AC101 ac;

/*
 * the main audio task
 */
inline void audio_task()
{
    memset(fl_sample, 0, sizeof(fl_sample));
    memset(fr_sample, 0, sizeof(fr_sample));
    memset(m1_sample, 0, sizeof(m1_sample));

#ifdef ESP32_AUDIO_KIT
    i2s_read_stereo_samples_buff(fl_sample, fr_sample, SAMPLE_BUFFER_SIZE);
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
    static uint32_t cycl = ESP.getCycleCount();
    static uint32_t lastCycl;

    lastCycl = cycl;
#ifdef ESP32_AUDIO_KIT
    button_loop();
#endif
    Blink_Process();
}

/*
 * this is the main loop
 */
void loop()
{
    audio_task(); /* audio tasks blocks for one sample -> 1/44100s */

    static uint32_t loop_cnt;

    loop_cnt += SAMPLE_BUFFER_SIZE;
    if ((loop_cnt) >= SAMPLE_RATE)
    {
        loop_cnt = 0;
        loop_1Hz();
    }

    /*
     * doing midi only 64 times per sample cycle
     */
    Midi_Process();

    //Console_Process();
}
