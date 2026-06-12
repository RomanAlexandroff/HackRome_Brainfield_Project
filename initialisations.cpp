/* ********************************************************************************************** */
/*                                                                                                */
/*   Flower Happiness Detector                                         :::::::::        :::       */
/*   initialisations.cpp                                              :+:    :+:     :+: :+:      */
/*                                                                   +:+    +:+    +:+   +:+      */
/*   By: Roman Alexandrov <r.aleksandroff@gmail.com>                +#++:++#:    +#++:++#++:      */
/*                                                                 +#+    +#+   +#+     +#+       */
/*   Created: 2024/05/21 12:50:00                                 #+#    #+#   #+#     #+#        */
/*   Updated: 2024/05/21 18:45:00                                ###    ###   ###     ###         */
/*                                                                                                */
/* ********************************************************************************************** */

#include "Flower_Happiness_Detector.h"

void IRAM_ATTR  serial_init(void)
{
    uint8_t i;
    
    i = 15;
    Serial.begin(115200);
    while (i)
    {
        DEBUG_PRINTF("-", "");
        delay(100);
        i--;
    }
    DEBUG_PRINTF("\n\nDEVICE START\nversion %s\n\n", String(SOFTWARE_VERSION));
}

void IRAM_ATTR  spiffs_init(void)
{
    short i;

    i = 3;
    while (!LittleFS.begin(true) && i)
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] Failed to initialise SPIFFS. Retrying...\n", "");
        delay(1000);
        i--;
    }
    if (!i)
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] SPIFFS was not initialised. Reading and Writing data is unavailable this session.\n", "");
        return ;
    }
    else
    {
        DEBUG_PRINTF("\n[FILE SYSTEM] SPIFFS is successfully initialised.\n", "");
        files_restore();
    }
    
}

int IRAM_ATTR wifi_connect(void)
{
    if(wifiMulti.run() != WL_CONNECTED)
        return (wifiMulti.run());
    return (WL_CONNECTED);
}

void IRAM_ATTR  wifi_init(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    wifiMulti.addAP(SSID1, PASSWORD1);
    wifiMulti.addAP(SSID2, PASSWORD2);
    wifiMulti.addAP(SSID3, PASSWORD3);
}

void IRAM_ATTR  adc_init(void)
{
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_11db);
}

void IRAM_ATTR temp_sensor_init(void)
{
    temp_sensor_config_t  temp_sensor;

    temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor.dac_offset = TSENS_DAC_L2;                // TSENS_DAC_L2 is default; L4(-40°C ~ 20°C), L2(-10°C ~ 80°C), L1(20°C ~ 100°C), L0(50°C ~ 125°C)
    temp_sensor_set_config(temp_sensor);
}
 
