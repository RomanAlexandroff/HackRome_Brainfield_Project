/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   file_system.cpp                                                  :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/23 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/23 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

short IRAM_ATTR  write_spiffs_file(const char* file_name, String input)
{
    short i;

    i = 1;
    File file = LittleFS.open(file_name, "w");
    while (!file && i <= 5)
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] An error occurred while opening %s file for writing in LittleFS. Retry.\n", file_name);
        file = LittleFS.open(file_name, "w");
        i++;
        delay(100);
    }
    if (!file)
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] Failed to open %s file for writing in LittleFS. Its dependant function will be unavailable during this programm cycle.\n", file_name);
        return (0);
    }
    else
    {
        file.println(input);
        file.close();
    }
    return (1);
}

String IRAM_ATTR  read_spiffs_file(const char* file_name)
{
    short  i;
    String output;

    i = 1;
    File file = LittleFS.open(file_name, "r");
    while (!file && i <= 5)
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] An error occurred while opening %s file for reading in LittleFS. Retry.\n", file_name);
        file = LittleFS.open(file_name, "r");
        i++;
        delay(100);
    }
    if (!file)
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] Failed to open %s file for reading in LittleFS. Its dependant function will be unavailable during this programm cycle.\n", file_name);
        file.close();
    }  
    else
    {
        output = file.readStringUntil('\n'); 
    }
    return (output);
}

void IRAM_ATTR  files_restore(void)
{
    if (!LittleFS.exists("/ota.txt"))
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] The ota.txt file does not exist. Creating...\n", "");
        write_spiffs_file("/ota.txt", CLOSED);
        DEBUG_PRINTF("[FILE SYSTEM] ota.txt file created. The rtc_g.ota value is recorded as %d\n", read_spiffs_file("/ota.txt").toInt() != 0);
    }
    rtc_g.ota = read_spiffs_file("/ota.txt").toInt() != 0;
    DEBUG_PRINTF("[FILE SYSTEM] The rtc_g.ota variable has been set to %d\n", rtc_g.ota);
    if (!LittleFS.exists("/chat_id.txt"))
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] The chat_id.txt file does not exist. Creating...\n", "");
        write_spiffs_file("/chat_id.txt", CHAT_ID);
        DEBUG_PRINTF("[FILE SYSTEM] chat_id.txt file created. The rtc_g.chat_id value is recorded as %d\n", read_spiffs_file("/chat_id.txt"));
    }
    rtc_g.chat_id = read_spiffs_file("/chat_id.txt");
    rtc_g.chat_id.trim();
    DEBUG_PRINTF("[FILE SYSTEM] The rtc_g.chat_id variable has been set to %d\n", rtc_g.chat_id);
}

 
