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

/**
 * @file pressure_module.ino
 * @author Marcel Licence
 * @date 14.04.2022
 *
 * @brief This file contains a rough implementation to control the FM Synth by a pressure sensor
 *
 * @see https://youtu.be/oHT1c7EdinU
 */


#ifdef PRESSURE_SENSOR_ENABLED

#include <Q2HX711.h> /* requires library Q2-HX711-Arduino-Library from https://github.com/queuetue/Q2-HX711-Arduino-Library */

#define PS_SDA  23
#define PS_SCL  18

//#define PRINT_PRESSURE /* activate this to show measured pressure value */

Q2HX711  MPS20N0040D(PS_SDA, PS_SCL); // start comm with the HX710B

void PressureSetup()
{
    Serial.printf("Setup pressure sensor, SDA: %d, SCL: %d\n", PS_SDA, PS_SCL);
    MPS20N0040D.setGain(64);
}

float PressConvert(uint32_t pv)
{
    uint32_t pl = 114300000;
    uint32_t ph = 220000000;

#ifdef PRINT_PRESSURE
    Serial.printf("pressure: %d\n", pv);
#endif

    if (pv < pl)
    {
        return 0.0001f;
    }
    if (pv > ph)
    {
        return 1.0f;
    }

    pv -= pl;
    return 1.0f * ((float)pv / (float)(ph - pl));
}

void PressureLoop()
{

    if (MPS20N0040D.readyToSend())
    {
        union
        {
            float fl;
            uint32_t lo;
            int32_t li;
        } ar;


        ar.lo = MPS20N0040D.read(); // add multiple ADC readings

        ar.lo <<= 9;

        float pf = PressConvert(ar.lo);
        FmSynth_Pressure(0, pf);
    }
}

#endif  /* PRESSURE_SENSOR_ENABLED */
