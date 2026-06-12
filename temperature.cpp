/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   temperature.cpp                                                  :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/22 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/22 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

static void temperature_calibration(void)
{
    int8_t  i;
    float   temperature;

    i = 0;
    temp_sensor_read_celsius(&temperature);
    while (temperature > 130.00 && i < 20)
    {
        temp_sensor_read_celsius(&temperature);
        delay(500);
        i++;
        DEBUG_PRINTF("\n[TEMPERATURE SENSOR] Temperature calibration in process, iteration #%d", i);
    }
}

static void temperature_sensor(int8_t status)
{
    if (status == ON)
        temp_sensor_start();
    if (status == OFF)
        temp_sensor_stop();
}

void  temperature_check(void)
{
    int8_t  i;
    float   total;
    float   temperature;

    i = 0;
    total = 0;
    temperature = 0;
    temperature_sensor(ON);
    temperature_calibration();
    while (i < 5)
    {
        temp_sensor_read_celsius(&temperature);
        total += temperature;
        delay(500);
        i++;
    }
    temperature_sensor(OFF);
    rtc_g.temp = total / i;
    DEBUG_PRINTF("\n[TEMPERATURE SENSOR] Current temperature: %.2f°C\n", rtc_g.temp);
}
