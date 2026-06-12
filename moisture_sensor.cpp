/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   moisture_sensor.cpp                                              :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/21 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/21 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

static void moisture_sensor(int8_t status)
{
    if (status == ON)
    {
        pinMode(SENSOR_GND, OUTPUT);
        pinMode(SENSOR_VCC, OUTPUT);
        digitalWrite(SENSOR_GND, LOW);
        delay(50);
        digitalWrite(SENSOR_VCC, HIGH);
        delay(50);
    }
    if (status == OFF)
    {
        digitalWrite(SENSOR_VCC, LOW);
        delay(50);
    }
}

void  moisture_check(void)
{
    int8_t   i;
    uint16_t moisture;

    i = 0;
    moisture = 0;
    moisture_sensor(ON);
    while (i < 5)
    {
        moisture += adc1_get_raw(ADC1_CHANNEL_3);
        delay(500);
        i++;
    }
    moisture_sensor(OFF);
    rtc_g.moisture = moisture / i;
    DEBUG_PRINTF("\n[MOISTURE SENSOR] Current moisture evaluation: %d\n\n", rtc_g.moisture);
}
 
