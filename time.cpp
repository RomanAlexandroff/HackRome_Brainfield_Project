/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   time.cpp                                                         :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/22 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/23 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

bool IRAM_ATTR  get_time(void)
{
    const char* ntp_server PROGMEM = "pool.ntp.org";
    const long  gmt_offset_sec = TIME_ZONE * 3600;
    const int   daylight_offset_sec PROGMEM = 3600;
    uint8_t     got_connection;
    bool        got_time;
    uint8_t     i;
    struct tm   time_info;

    i = 0;
    got_connection = 0;
    while (got_connection != WL_CONNECTED && i <= 5)
    {
        got_connection = wifi_connect();
        i++;
        DEBUG_PRINTF("Establishing Wi-Fi connection, try #%d\n", i);
        delay(1000);
    }
    if (got_connection != WL_CONNECTED)
    {
        DEBUG_PRINTF("Failed to obtain time due to Wi-Fi connection issues\n", "");
        return (false);
    }
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
    i = 0;
    got_time = false;
    while (!got_time && i <= 5)
    {
        got_time = getLocalTime(&time_info);
        i++;
        DEBUG_PRINTF("Obtaining time from the NTP server, try #%d\n", i);
        delay(1000);
    }
    if (!got_time)
    {
        DEBUG_PRINTF("Failed to obtain time from the NTP server\n", "");
        return (false);
    }
    rtc_g.hour = time_info.tm_hour;
    rtc_g.minute = time_info.tm_min;
    rtc_g.day = time_info.tm_mday;
    rtc_g.month = 1 + time_info.tm_mon;
    rtc_g.year = 1900 + time_info.tm_year;
    DEBUG_PRINTF("\nObtained time from the NTP server is as follows:\n", "");
    DEBUG_PRINTF("  --hour:   %d\n", rtc_g.hour);
    DEBUG_PRINTF("  --minute: %d\n", rtc_g.minute);
    DEBUG_PRINTF("  --day:    %d\n", rtc_g.day);
    DEBUG_PRINTF("  --month:  %d\n", rtc_g.month);
    DEBUG_PRINTF("  --year:   %d\n\n", rtc_g.year);
    return (true);
}

unsigned int  time_till_wakeup(void)
{
    const uint8_t wakeup_hour[] = {8, 10, 12, 14, 16, 18, 20};
    uint8_t       i;

    if (rtc_g.hour >= 20)
        return ((wakeup_hour[0] + 24 - rtc_g.hour) * 3600000 - (rtc_g.minute * 60000) - millis());
    i = 0;
    while ((wakeup_hour[i] - rtc_g.hour) <= 0)
        i++;
    return ((wakeup_hour[i] - rtc_g.hour) * 3600000 - (rtc_g.minute * 60000) - millis());
}
 
