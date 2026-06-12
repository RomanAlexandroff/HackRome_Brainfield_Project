/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   battery.cpp                                                      :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/21 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/21 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

void  low_battery_handle(void)
{
    DEBUG_PRINTF("\n[BATTERY] Low battery! Cannot proceed work. Need charging\n", "");
    bot.sendMessage(rtc_g.chat_id, "The sensor's battery is low! Need charging. Turning off", "");
    go_to_sleep(DEAD_BATTERY_SLEEP);
}

bool charging_detection(void)
{
    uint32_t start_time;
    uint32_t end_time;

    start_time = millis();
    while (rtc_g.temp > 40.00)
    {
        temperature_check();
    }
    end_time = millis() - start_time;
    
/*    short i;
    int   new_result;
    int   old_result;
    int   counter;

    i = 5;
    counter = 0;
    old_result = rtc_g.battery;
    while(i)
    {
        new_result = adc1_get_raw(ADC1_CHANNEL_0);
        if (new_result < 550)
            return (false);
        if ((old_result - new_result) > 0)
            counter++;
        old_result = new_result;
        i--;
        delay(60000);
    }
    if (counter >= 4)
        return (false); */
    return (true);
}

void  battery_check(void)
{
    int8_t   i;
    uint16_t battery;

    i = 0;
    battery = 0;
    while (i < 10)
    {
        battery += adc1_get_raw(ADC1_CHANNEL_0);
        delay(100);
        i++;
    }
    rtc_g.battery = battery / i;
    DEBUG_PRINTF("\n[BATTERY] Current battery state: %d\n", rtc_g.battery);
}
 
