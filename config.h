/*
 * The GNU GENERAL PUBLIC LICENSE (GNU GPLv3)
 *
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

//#define AUDIO_PASS_THROUGH /* can be used to pass line in through audio processing to output */
/* use following when you are using the esp32 audio kit v2.2 */
#define ESP32_AUDIO_KIT /* project has not been tested on other hardware, modify on own risk */
//#define ES8388_ENABLED /* use this if the Audio Kit is equipped with ES8388 instead of the AC101 */


/*
 * configuration for ESP32 Audio Kit
 * enable ESP32_AUDIO_KIT when using the ESP32 Audio Kit
 */
#ifdef ESP32_AUDIO_KIT

/* on board led */
#ifdef ESP32_AUDIO_KIT
#define BLINK_LED_PIN     12
#else
#define BLINK_LED_PIN     19 // IO19 -> D5
#endif

#ifdef ES8388_ENABLED
/* i2c shared with codec */
#define I2C_SDA 18
#define I2C_SCL 23
#endif

#else /* ESP32_AUDIO_KIT */

/* on board led */
#define BLINK_LED_PIN     2

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

//#define MIDI_VIA_USB_ENABLED

/*
 * pins used for MIDI
 */
#ifdef ESP32_AUDIO_KIT
#ifdef ES8388_ENABLED
#define MIDI_RX_PIN 19
#else
#define MIDI_RX_PIN 19
//#define TXD2 21
#endif
#else
#define MIDI_RX_PIN 19 /* U2RRXD */
//#define TXD2 17
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
