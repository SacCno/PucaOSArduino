#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

#include "config.h"

extern "C" void printSerial(const char *fmt, ...);
static bool provisional_mqtt_password();

extern "C" bool pucaos_platform_init(void)
{
    printSerial("Entered platform init");

    if (!LittleFS.begin(false))
    {
        printSerial("LittleFS mount failed");
        return false;
    }

    provisional_mqtt_password();

    printSerial("LittleFS mounted");

    File root = LittleFS.open("/");

    if (!root)
    {
        printSerial("Could not open LittleFS root");
    }
    else if (!root.isDirectory())
    {
        printSerial("LittleFS root is not a directory");
    }
    else
    {
        bool found_file = false;
        File file = root.openNextFile();

        while (file)
        {
            found_file = true;
            printSerial("Found: %s (%u bytes)",
                        file.name(),
                        (unsigned)file.size());

            file = root.openNextFile();
        }

        if (!found_file)
        {
            printSerial("LittleFS is empty");
        }
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.disconnect();
    delay(100);
    printSerial("Connecting to WiFi SSID '%s'", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    delay(5000);

    const unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000UL)
    {
        delay(250);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        printSerial("WiFi connection failed, status=%d", (int)WiFi.status());

        int network_count = WiFi.scanNetworks();
        printSerial("WiFi scan found %d networks", network_count);
        for (int network_index = 0; network_index < network_count; ++network_index)
        {
            if (WiFi.SSID(network_index) == WIFI_SSID)
            {
                printSerial("Target SSID visible, RSSI=%d dBm, encryption=%d",
                            WiFi.RSSI(network_index),
                            (int)WiFi.encryptionType(network_index));
            }
        }

        return false;
    }

    configTzTime("GMT0IST,M3.5.0/1,M10.5.0/2", "pool.ntp.org", "time.nist.gov");

    delay(500);

    printSerial("Setup Complete");

    return true;
}

extern "C" void printSerial(const char *fmt, ...)
{
    if (fmt == NULL)
    {
        return;
    }
    if (!Serial)
    {
        return;
    }

    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    Serial.println(buffer);
}

static bool provisional_mqtt_password()
{
    if (LittleFS.exists("/mqttpassword.txt"))
    {
        printSerial("MQTT password file found");
        return true;
    }

    printSerial("Enter MQTT password, then press enter...");

    while (!Serial.available())
    {
        delay(50);
    }

    String password = Serial.readStringUntil('\n');

    if (password.endsWith("\r"))
    {
        password.remove(password.length() - 1);
    }

    if (password.length() == 0 || password.length() >= 32)
    {
        printSerial("Invalid password length; must be 1-31 characters");
        return false;
    }

    File file = LittleFS.open("/mqttpassword.txt", "w");

    if (!file)
    {
        printSerial("Could not create password file");
        return false;
    }

    file.print(password);
    file.close();

    printSerial("MQTT password saved");
    return true;
}

extern "C" bool password_reader(
    const char *path,
    char *output,
    size_t output_size)
{
    if (path == nullptr || output == nullptr || output_size < 2)
    {
        return false;
    }

    File file = LittleFS.open(path, "r");

    if (!file)
    {
        return false;
    }

    size_t length = file.readBytesUntil('\n', output, output_size - 1);
    file.close();

    output[length] = '\0';

    if (length > 0 && output[length - 1] == '\r')
    {
        output[length - 1] = '\0';
    }

    return output[0] != '\0';
}

extern "C" void arduino_serial_hexdump(const void *buf, size_t len)
{
    const uint8_t *data = (const uint8_t *)buf;
    printSerial("----------------------------");
    for (size_t i = 0; i < len; i++)
    {
        if (data[i] < 0x10)
        {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");

        if ((i + 1) & 16 == 0)
        {
            printSerial("");
        }
    }

    printSerial("\n------------------------------");
}