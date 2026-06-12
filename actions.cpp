/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   actions.cpp                                                      :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/21 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/21 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"


void IRAM_ATTR  react(void)
{
    if (rtc_g.battery <= 875 && rtc_g.temp < 39.00)               // potentially low battery and no charging — switching off
        low_battery_handle();
    if (rtc_g.moisture > 3100 && rtc_g.temp >= 40.00)             // potentially charging is in action — no work, waiting for charging to be complete
        charging_detection();
}


int IRAM_ATTR  how_moist(void)
{
    return (ceil((float)(3180 - rtc_g.moisture) / 1870.0f * 100.0f));
}


void IRAM_ATTR  notify_user(void)
{
    String  message;

    if (wifi_connect() != WL_CONNECTED)
    {
        DEBUG_PRINTF("Could not notify User due to Wi-Fi connection issues\n", "");
        return;
    }
    if (rtc_g.moisture >= 1310 && rtc_g.moisture <= 1870 && (rtc_g.last_notification != TOO_MUCH))
    {
        DEBUG_PRINTF("\nTHE PLANT SAYS: That's too much water!\n", "");
        message = "That's too much water! The soil moisture's at ";
        message += String(how_moist()) + "%!";
        bot.sendMessage(rtc_g.chat_id, message, "");
        rtc_g.last_notification = TOO_MUCH;
        return;
    }
    if (rtc_g.moisture >= 1871 && rtc_g.moisture <= 2245 && (rtc_g.last_notification != JUST_OK))
    {
        DEBUG_PRINTF("\nTHE PLANT SAYS: I am happy =)\n", "");
        bot.sendMessage(rtc_g.chat_id, "I am happy =)", "");
        rtc_g.last_notification = JUST_OK;
        return;
    }
    if (rtc_g.moisture >= 2246 && rtc_g.moisture <= 2432)
    {
        DEBUG_PRINTF("\nTHE PLANT SAYS: It's time to water me\n", "");
        message = "It's time to water me. The soil moisture's got down to ";
        message += String(how_moist()) + "%";
        bot.sendMessage(rtc_g.chat_id, message, "");
        rtc_g.last_notification = NEED_WATER;
        return;
    }
    if (rtc_g.moisture >= 2433 && rtc_g.moisture <= 3100)
    {
        DEBUG_PRINTF("\nTHE PLANT SAYS: I'm dying here! Water me!!!\n", "");
        message = "I've got only " + String(how_moist()) + "% of water left! ";
        message += "I'm dying here! Water me now!";
        bot.sendMessage(rtc_g.chat_id, message, "");
        rtc_g.last_notification = DRYING_OUT;
        return;
    }
}
 
