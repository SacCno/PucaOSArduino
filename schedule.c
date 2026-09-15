#include "schedule.h"
#include <stdio.h>
#include <stdlib.h>

#define SCHEDULE_FILE "/schedule.bin" // PC solution, ESP will use flash

static void sort_schedule(Schedule *s);

const Schedule default_schedule =
    {
        .magic = 0x53434831,
        .version = 1,
        .count = 3,
        .timeframes = {
            {300, 360, 1}, // 30.0°C at 06:00 lamp on
            {350, 720, 1}, // 35.0°C at 12:00 lamp on
            {250, 1260, 0} // 25.0°C at 21:00 lamp off
        },
        .crc = 0};

uint16_t get_temperature(const Schedule *sch, uint16_t current_minute)
{
  if (sch == NULL || current_minute > 1439)
  {
    printf("get_temperature: Invalid entry parameters.");
    return 0;
  }

  int active = -1;

  for (uint16_t i = 0; i < sch->count; i++)
  {
    if (sch->timeframes[i].minute_of_day <= current_minute)
    {
      active = i;
    }
    else
    {
      break;
    }
  }

  if (active < 0 && sch->count > 0)
  {
    active = sch->count - 1;
  }

  return active >= 0 ? sch->timeframes[active].temperature : 0;
}

uint16_t get_lamp_state(const Schedule *sch, uint16_t current_minute)
{
  if (sch == NULL || current_minute > 1439)
  {
    printf("get_lamp_state: Invalid entry parameters.");
    return 0;
  }

  int active = -1;

  for (uint16_t i = 0; i < sch->count; i++)
  {
    if (sch->timeframes[i].minute_of_day <= current_minute)
    {
      active = i;
    }
    else
    {
      break;
    }
  }

  if (active < 0 && sch->count > 0)
  {
    active = sch->count - 1;
  }

  return active >= 0 ? sch->timeframes[active].lamp_on : 0;
}

bool validate_schedule(Schedule *s)
{
  if (s == NULL)
  {
    printf("Cannot validate null schedule.\n");
    return false;
  }

  if (s->count > MAX_TIMEFRAMES)
  {
    printf("Error: Too many timeframes.");
    return false;
  }

  sort_schedule(s);

  for (uint16_t i = 0; i < s->count; i++)
  {
    if (s->timeframes[i].minute_of_day >= 24 * 60)
    {
      printf("Error: Minutes of day invalid");
      return false;
    }

    if (s->timeframes[i].temperature < 50 || s->timeframes[i].temperature > 600)
    {
      printf("Error: Temperature out of range. temp=%u (%0.1fC)\n",
             (unsigned)s->timeframes[i].temperature,
             s->timeframes[i].temperature / 10.0);
      return false;
    }

    if (i > 0 && s->timeframes[i - 1].minute_of_day >= s->timeframes[i].minute_of_day)
    {
      printf("Error: Timeframe out of order or overlapping.");
      return false;
    }

    if (s->timeframes[i].lamp_on != 1 && s->timeframes[i].lamp_on != 0)
    {
      printf("Error: Lamp Int not 1 or 0");
      return false;
    }
  }

  return true;
}

static int compare(const void *a, const void *b)
{
  Timeframe tf_a = *((Timeframe *)a);
  Timeframe tf_b = *((Timeframe *)b);

  if (tf_a.minute_of_day == tf_b.minute_of_day)
    return 0;
  else if (tf_a.minute_of_day < tf_b.minute_of_day)
    return -1;
  else
    return 1;
}

static void sort_schedule(Schedule *s)
{
  qsort(s->timeframes, s->count, sizeof(Timeframe), compare);
}

bool save_schedule(const Schedule *schedule)
{
  if (schedule == NULL)
  {
    printf("Cannot save a null schedule.\n");
    return false;
  }

  FILE *file = fopen(SCHEDULE_FILE, "wb");

  if (file == NULL)
  {
    printf("Failed to save schedule.");
    return false;
  }

  size_t written = fwrite(schedule, sizeof(Schedule), 1, file);
  fclose(file);

  if (written == 0)
  {
    printf("Failed to save schedule.\n");
  }
  else
  {
    printf("Saved schedule successfully.\n");
  }

  return written == 1;
}

static bool load_schedule(Schedule *schedule)
{
  if (schedule == NULL)
  {
    printf("Cannot load into a null schedule.\n");
    return false;
  }

  FILE *file = fopen(SCHEDULE_FILE, "rb");

  if (file == NULL)
  {
    printf("File error.");
    return false;
  }

  size_t read = fread(schedule, sizeof(Schedule), 1, file);
  fclose(file);

  return read == 1;
}

bool file_start_up(Schedule *schedule)
{
  if (schedule == NULL)
  {
    printf("Cannot load into a null schedule.\n");
    return false;
  }

  if (!load_schedule(schedule))
  {
    printf("No saved schedule found; using defaults.\n");
    *schedule = default_schedule;
    save_schedule(schedule);
    return false;
  }
  else
  {
    printf("Schedule Loaded Successfully.\n");
  }

  if (schedule->count == 0)
  {
    *schedule = default_schedule;
  }

  if (!validate_schedule(schedule))
  {
    printf("Invalid schedule loaded; using defaults instead.\n");
    *schedule = default_schedule;
  }

  save_schedule(schedule);

  return true;
}