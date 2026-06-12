/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   Flower_Happiness_Detector.ino                                    :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/16 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/16 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/*                                                                                                */
/*   This software allows to track moisture of plants via Telegram chat notifications.            */
/*                                                                                                */
/*   This project is interesting because:                                                         */
/*      - here I use a digital GPIO to source power to a sensor. Effectively, it allows me        */
/*        to turn the sensor's power on and off without soldering external MOSFETs.               */
/*      - here I use another digital GPIO as ground. I had to go this way due to some physical    */
/*        restrictions of the boards used in the project. This solution is far from perfect,      */
/*        because later experiments showed that connecting sensor's GND to the proper ground      */
/*        would have provided more voltage for the sensor.                                        */
/*      - it's the first time I use inner temperature sensor of ESP32.                            */
/*      - it's the first time I managed to start OTA functionality by a Telegram chat command.    */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

void  setup(void)
{
    #ifdef DEBUG
        serial_init();
    #endif
    spiffs_init();
    wifi_init();                                 // sets up the Wi-Fi module but does not connect
    adc_init();
    temp_sensor_init();
    ota_init();
}

void  loop(void)
{
    ota_waiting_loop();
    temperature_check();
    battery_check();
    moisture_check();
    react();
    get_time();
    notify_user();
    telegram_check();
    go_to_sleep(time_till_wakeup());
}
 
