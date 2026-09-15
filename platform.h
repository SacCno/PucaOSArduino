#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool pucaos_platform_init(void);
void printSerial(const char *msg);

#ifdef __cplusplus
}
#endif

#endif