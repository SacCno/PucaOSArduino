#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"

extern "C" bool pucaos_platform_init(void)
{
    Serial.begin(115200);
    delay(500);
    Serial.println("Entered platform init");

    if(!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, WIFI_PASS);

    const unsigned long start = millis();

    while(WiFi.status() != WL_CONNECTED && millis() - start < 20000UL)
    {
        delay(250);
    }

    if(WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection failed");
        return false;
    }

    configTzTime("GMT0IST,M3.5.0/1,M10.5.0/2", "pool.ntp.org", "time.nist.gov");

    delay(500);

    Serial.println("Setup Complete");

    return true;
}

extern "C" void printSerial(const char * msg)
{
    Serial.println(msg);
}