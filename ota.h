/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   ota.h                                                            :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/19 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/24 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/*                                                                                                */
/*   This file contains inline functions declared in the flower_happiness_detector.h header.      */
/*   This file has to be included AFTER all the functions' declarations.                          */
/*                                                                                                */
/* ********************************************************************************************** */

#ifndef OTA_H
# define OTA_H

void ota_init(void)
{
    if (rtc_g.ota)
    {
        uint8_t i = 0;
        uint8_t got_connection = 0;
        char    fullhostname[26] = "Flower_Happiness_Detector";
        while (got_connection != WL_CONNECTED && i <= 5)
        {
            got_connection = wifi_connect();
            i++;
            DEBUG_PRINTF("\n[OTA] Establishing Wi-Fi connection, try #%d\n", i);
            delay(1000);
        }
        if (got_connection != WL_CONNECTED)
        {
            DEBUG_PRINTF("\n[OTA] Failed to initialise OTA update due to Wi-Fi connection issues\n", "");
            return;
        }
        ArduinoOTA.setHostname(fullhostname);
        ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";
                DEBUG_PRINTF("[OTA] Start updating %s", type.c_str());
                bot.sendMessage(rtc_g.chat_id, "Updating...", "");
            })
            .onEnd([]() {
                DEBUG_PRINTF("\n[OTA] End", "");
                rtc_g.ota = false;
                write_spiffs_file("/ota.txt", CLOSED);
                bot.sendMessage(rtc_g.chat_id, "Successfully updated!", "");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                DEBUG_PRINTF("\n[OTA] Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                DEBUG_PRINTF("\n[OTA] Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) DEBUG_PRINTF("Auth Failed\n", "");
                else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTF("Begin Failed\n", "");
                else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTF("Connect Failed\n", "");
                else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTF("Receive Failed\n", "");
                else if (error == OTA_END_ERROR) DEBUG_PRINTF("End Failed\n", "");
                bot.sendMessage(rtc_g.chat_id, "Something went wrong. Updating was not completed. Try again later", "");
            });
        DEBUG_PRINTF("\n[OTA] Ready to update\n\n", "");
        bot.sendMessage(rtc_g.chat_id, "OTA Update is active", "");
        ArduinoOTA.begin();
    }
}

void ota_waiting_loop(void)
{
    if (rtc_g.ota)
    {
        uint16_t ota_limit = 0;
        while (rtc_g.ota && ota_limit < 1000)                                       // 1000 gives 5 minutes to perform OTA
        {
            ArduinoOTA.handle();
            DEBUG_PRINTF("\n[OTA] Active", "");
            ota_limit++;
            delay(300);
        }
        rtc_g.ota = false;
        write_spiffs_file("/ota.txt", CLOSED);
        bot.sendMessage(rtc_g.chat_id, "OTA Update port closed", "");
        DEBUG_PRINTF("\n[OTA] OTA Update port closed", "");
    }
}

#endif
 
