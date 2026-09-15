#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TIMEFRAMES 12

typedef struct Timeframe
{
  uint16_t temperature;   // Measured in tenths of a degree -      160 = 16.0C     123 = 12.3C
  uint16_t minute_of_day; // Minutes since midnight
  uint16_t lamp_on;
} Timeframe;

typedef struct Schedule
{
  uint32_t magic;
  uint16_t version;
  uint16_t count;
  Timeframe timeframes[MAX_TIMEFRAMES];
  uint32_t crc;
} Schedule;

extern const Schedule default_schedule;

bool file_start_up(Schedule *schedule);
bool save_schedule(const Schedule *schedule);
uint16_t get_temperature(const Schedule *sch, uint16_t current_minute);
uint16_t get_lamp_state(const Schedule *sch, uint16_t current_minute);
bool validate_schedule(Schedule *s);

#endif