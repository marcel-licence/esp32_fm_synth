/*
 * config.h
 *
 * Put all your project settings here (defines, numbers, etc.)
 * configurations which are requiring knowledge of types etc.
 * shall be placed in z_config.ino (will be included at the end)
 *
 *  Created on: 12.05.2021
 *      Author: Marcel Licence
 */

#ifndef CONFIG_H_
#define CONFIG_H_

//#define ESP32_AUDIO_KIT


/*
 * configuration for ESP32 Audio Kit
 * enable ESP32_AUDIO_KIT when using the ESP32 Audio Kit
 */
#ifdef ESP32_AUDIO_KIT

/* on board led */
#define LED_PIN     19 // IO19 -> D5

#else /* ESP32_AUDIO_KIT */

/* on board led */
#define LED_PIN     2

/*
 * Define and connect your PINS to DAC here
 */
#ifdef I2S_NODAC
#define I2S_NODAC_OUT_PIN   22  /* noisy sound without DAC, add capacitor in series! */
#else
/*
 * pins to connect a real DAC like PCM5201
 */
#define I2S_BCLK_PIN    25
#define I2S_WCLK_PIN    27
#define I2S_DOUT_PIN    26
#endif

#endif /* ESP32_AUDIO_KIT */

/*
 * allows to receive MIDI messages from serial line
 * useful when using a pass through via computer
 * - USB MIDI in -> UART serial out
 */
#define MIDI_RECV_FROM_SERIAL

/*
 * pins used for MIDI
 */
#ifdef ESP32_AUDIO_KIT
#define RXD2 22
#define TXD2 21
#else
#define RXD2 16 /* U2RRXD */
#define TXD2 17
#endif

#ifdef ESP32_AUDIO_KIT
#define SAMPLE_BUFFER_SIZE  48
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE_16BIT
#else
#define SAMPLE_BUFFER_SIZE  48
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE_16BIT
#endif

#endif /* CONFIG_H_ */
