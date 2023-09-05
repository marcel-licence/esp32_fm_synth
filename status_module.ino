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
 * @file status_module.ino
 * @author Marcel Licence
 * @date 04.10.2021
 *
 * @brief This file contains the implementation of the terminal output
 * the output is vt100 compatible and you should use a terminal like teraTerm
 *
 * @see VT100 mode used here: https://youtu.be/r0af0DB1R68
 * @see TeraTerm pro https://ttssh2.osdn.jp/index.html.en​
 */


#ifdef __CDT_PARSER__
#include <cdt.h>
#endif


bool triggerTerminalOutput = true; /*!< necessary for usage without vt100 compliant terminal */

char statusMsg[128] = ""; /*!< buffer for top line message */

uint32_t statusMsgShowTimer = 0; /*!< counter for timeout to reset top line */

#define VU_MAX  24 /*!< character length of vu meter */

float statusVuLookup[VU_MAX]; /*!< precalculated lookup */

/*
 * prepares the module
 */
void Status_Setup(void)
{
    /*
     * prepare lookup for log vu meters
     */
    float vuValue = 1.0f;
    for (int i = VU_MAX; i > 0; i--)
    {
        int n = i - 1;

        statusVuLookup[n] = vuValue;

        vuValue *= 1.0f / sqrt(2.0f); /* div by 1/sqrt(2) means 3db */
        vuValue *= 1.0f / sqrt(2.0f);
    }
}

/*
 * function which prints the vu meter "line"
 */
void Status_PrintVu(float *value, uint8_t vuMax)
{
    /*
     * first pick a color
     */
    if (*value >= 0.7071f) /* -3dB */
    {
        Serial.printf("\033[0;31m"); /* red */
    }
    else if (*value >= 0.5) /* -6dB */
    {
        Serial.printf("\033[0;33m"); /* yellow */
    }
    else
    {
        Serial.printf("\033[0;32m"); /* green */
    }

    for (int i = 0; i < vuMax; i++)
    {

        if (statusVuLookup[i + (VU_MAX - vuMax)] <= *value)
        {
            Serial.printf("#");
        }
        else
        {
            Serial.printf(" ");
        }
    }

    Serial.printf("\033[0m"); /* switch back to default */

    *value *= 0.5; /* slowly lower the value */
}


/*
 * refresh complete output
 * 32 character width
 * 14 character height
 */
void Status_PrintAll(void)
{
    char emptyLine[] = "                                ";
#if 0 /* not used */
    char emptyLineMin[] = "                               ";
    char emptyLineLong[] = "                                                                ";
#endif
    Serial.printf("\033[?25l");
    Serial.printf("\033[%d;%dH", 0, 0);
    //Serial.printf("--------------------------------\n");
    Serial.printf("%s%s\n", statusMsg, &emptyLine[strlen(statusMsg)]);

}


void Status_Process_Sample(uint32_t inc)
{
    statusMsgShowTimer += inc;
    if (statusMsgShowTimer == SAMPLE_RATE * 3)
    {
        statusMsgShowTimer = SAMPLE_RATE * 3 + 1;
        statusMsg[0] = 0;
        Status_PrintAll();
    }
}

void Status_Process(void)
{
    if (triggerTerminalOutput)
    {
        Status_PrintAll();
        triggerTerminalOutput = false;
    }

    if (triggerTerminalOutput)
    {
        Status_PrintAll();
        triggerTerminalOutput = false;
    }
}

/*
 * update top line message including a float value
 */
void Status_ValueChangedFloat(const char *group, const char *descr, float value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s - %s: %0.3f", group, descr, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedFloat(const char *descr, float value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s: %0.3f", descr, value);
    triggerTerminalOutput = true;
}

/*
 * update top line message including a float value
 */
void Status_ValueChangedFloatArr(const char *descr, float value, int index)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s[%d]: %0.3f", descr, index, value);
    triggerTerminalOutput = true;
}

/*
 * update top line message including an integer value
 */
void Status_ValueChangedIntArr(const char *descr, int value, int index)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s[%d]: %d", descr, index, value);
    triggerTerminalOutput = true;
}

/*
 * update top line message including an integer value
 */
void Status_ValueChangedInt(const char *group, const char *descr, int value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s - %s: %d", group, descr, value);
    triggerTerminalOutput = true;
}

void Status_ValueChangedInt(const char *descr, int value)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s: %d", descr, value);
    triggerTerminalOutput = true;
}

/*
 * update top line message
 */
void Status_TestMsg(const char *text)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s", text);
    triggerTerminalOutput = true;
}

void Status_LogMessage(const char *text)
{
    statusMsgShowTimer = 0;
    sprintf(statusMsg, "%s", text);
    triggerTerminalOutput = true;
}

