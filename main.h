#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef struct {
  uint16_t lamp_status;
  uint16_t current_temp;
  uint16_t target_temp;
} Current_Stats;

#endif