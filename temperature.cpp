#include <OneWire.h>
#include <DallasTemperature.h>

#include "platform.h"
#include "temperature.h"

const int SENSOR_PIN = 5;

OneWire oneWire(SENSOR_PIN);

DallasTemperature sensors(&oneWire);

void init_temp()
{
    sensors.begin();
}

uint16_t sensor_temp()
{
    sensors.requestTemperatures();

    float temperature = sensors.getTempCByIndex(0);
    if (temperature == -127.00)
    {
        printSerial("Error: Cannot get temperature data.");
        return 0;
    }

    return (uint16_t)(temperature * 100);
}