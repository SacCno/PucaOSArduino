#include <OneWire.h>
#include <DallasTemperature.h>

#include "platform.h"

const int SENSOR_PIN = 5;

OneWire oneWire(SENSOR_PIN);

DallasTemperature sensors(&oneWire);

extern "C" void init_temp()
{
    sensors.begin();
}

extern "C" uint16_t sensor_temp()
{
    sensors.requestTemperatures();

    float temperature = sensors.getTempCByIndex(0);
    if (temperature == -127.00)
    {
        printSerial("Error: Cannot get temperature data.");
    }

    return (uint16_t)(temperature * 10);
}