/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   telegram_bot.cpp                                                 :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/22 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/22 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

static void reply_machine(String text)
{
    String      message;

    if (text == "/status")
    {
        message = "Connected to " + String(WiFi.SSID());   
        message += ", Signal strength is " + String(WiFi.RSSI()) + " dBm";
        message += ", Battery state: " + String(rtc_g.battery);
        message += ", Current core temperature: " + String(rtc_g.temp) + "°C";
        message += ", Current moisture evaluation: " + String(rtc_g.moisture);
        message += " which corresponds to " + String(how_moist()) + "%";
        message += ", Software version " + String(SOFTWARE_VERSION);
        bot.sendMessage(rtc_g.chat_id, message, "");
        message.clear();
        return;
    }
    else if (text == "/ota")
    {
        if (rtc_g.ota == false)
        {
            write_spiffs_file("/ota.txt", ACTIVE);
            rtc_g.reboot = true;
        }
        else
        {
            rtc_g.ota = false;
            write_spiffs_file("/ota.txt", CLOSED);
            rtc_g.reboot = false;
        }
        return;
    }
    else if (text == "/reboot")
    {
        bot.sendMessage(rtc_g.chat_id, "Rebooting!", "");
        rtc_g.reboot = true;
        return;
    }
    else
    {
        message = "I am sorry, but I do not understand \"";
        message += text + "\"\n\n";
        message = "You may try to use \"/status\" command";
        bot.sendMessage(rtc_g.chat_id, message, "");
    }
}

static void  new_messages(short message_count)
{
    uint8_t i;
    String  text;

    i = 0;
    DEBUG_PRINTF("\n\n[TELEGRAM BOT] Handling new Telegram messages", "");
    DEBUG_PRINTF("\n[TELEGRAM BOT] Number of messages to handle: %d\n", message_count);
    while (i < message_count) 
    {
        DEBUG_PRINTF("[TELEGRAM BOT] Handling loop iteration #%d\n", i + 1);
        rtc_g.chat_id = String(bot.messages[i].chat_id);
        write_spiffs_file("/chat_id.txt", rtc_g.chat_id);
        text = bot.messages[i].text;
        rtc_g.from_name = bot.messages[i].from_name;
        DEBUG_PRINTF("\n[TELEGRAM BOT] %s says: ", rtc_g.from_name.c_str());
        DEBUG_PRINTF("%s\n\n", text.c_str());
        reply_machine(text);
        i++;
    }
}

void IRAM_ATTR  telegram_check(void)
{
    uint8_t i;
    uint8_t got_connection;
    short   message_count;

    i = 0;
    got_connection = 0;
    while (got_connection != WL_CONNECTED && i <= 5)
    {
        got_connection = wifi_connect();
        i++;
        DEBUG_PRINTF("\n[TELEGRAM BOT] Establishing Wi-Fi connection, try #%d\n", i);
        delay(1000);
    }
    if (got_connection != WL_CONNECTED)
    {
        DEBUG_PRINTF("\n[TELEGRAM BOT] Failed to reply to the User due to Wi-Fi connection issues\n", "");
        return;
    }
    else
    {     
        message_count = bot.getUpdates(bot.last_message_received + 1);
        while (message_count)
        {
            new_messages(message_count);
            message_count = bot.getUpdates(bot.last_message_received + 1);
        }
    }
    if (rtc_g.reboot)
        ESP.restart();
}
 
