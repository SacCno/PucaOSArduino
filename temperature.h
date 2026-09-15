#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_temp(void);
uint16_t sensor_temp(void);

#ifdef __cplusplus
}
#endif

#endif