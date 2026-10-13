#include "schedule.h"
#include "platform.h"
#include "mongoose.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define SCHEDULE_FILE "/schedule.bin"

static void sort_schedule(Schedule *s);

uint32_t schedule_crc(Schedule *schedule)
{
  return mg_crc32(0, (const char *)schedule, offsetof(Schedule, crc));
}

bool schedule_init(Schedule *schedule, uint16_t version, Timeframe timeframes[], uint16_t count)
{
  if (schedule == NULL || count > MAX_TIMEFRAMES || count == 0 || timeframes == NULL)
  {
    printSerial("Schedule Init Error.");
    return false;
  }

  Schedule new = {0};

  new.magic = 0x53434831;
  new.version = version;
  new.count = count;

  memcpy(new.timeframes, timeframes,
         count * sizeof new.timeframes[0]);

  new.crc = schedule_crc(&new);

  if (!validate_schedule(&new))
  {
    printSerial("Schedule init error.");
    return false;
  }

  *schedule = new;
  return true;
}

bool schedule_init_default(Schedule *schedule)
{
  if (schedule == NULL)
  {
    printSerial("Schedule Init Error.");
    return false;
  }

  Schedule new = {0};

  new.magic = 0x53434831;
  new.version = 2;
  new.count = 3;

  Timeframe tfs[3] =
      {
          {3000, 360, 1}, // 30.0°C at 06:00 lamp on
          {3500, 720, 1}, // 35.0°C at 12:00 lamp on
          {2500, 1260, 0} // 25.0°C at 21:00 lamp off
  };

  memcpy(new.timeframes, tfs,
         new.count * sizeof new.timeframes[0]);

  new.crc = schedule_crc(&new);

  *schedule = new;
  return true;
}

uint16_t get_temperature_from_time(const Schedule *sch, uint16_t current_minute)
{
  if (sch == NULL || current_minute > 1439)
  {
    printSerial("get_temperature: Invalid entry parameters.");
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
    printSerial("get_lamp_state: Invalid entry parameters.");
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

bool check_crc(Schedule *s)
{
  return s != NULL && s->crc == schedule_crc(s);
}

bool check_count(Schedule *s)
{
  return s != NULL && s->count > 0 && s->count <= MAX_TIMEFRAMES;
}

bool validate_schedule(Schedule *s)
{
  if (s == NULL)
  {
    printSerial("Cannot validate null schedule.");
    return false;
  }

  if (!check_count(s))
  {
    printSerial("Error: Too many timeframes.");
    return false;
  }

  if (s->magic != 0x53434831)
  {
    printSerial("Error: Invalid schedule magic number.");
    return false;
  }

  if (!check_crc(s))
  {
    printSerial("Error: Incorrect CRC");
    return false;
  }

  sort_schedule(s);

  s->crc = schedule_crc(s);

  for (uint16_t i = 0; i < s->count; i++)
  {
    if (s->timeframes[i].minute_of_day >= 24 * 60)
    {
      printSerial("Error: Minutes of day invalid");
      return false;
    }

    if (s->timeframes[i].temperature < 500 || s->timeframes[i].temperature > 6000)
    {
      printSerial("Error: Temperature out of range. temp=%u (%0.2fC)",
                  (unsigned)s->timeframes[i].temperature,
                  s->timeframes[i].temperature / 100.0);
      return false;
    }

    if (i > 0 && s->timeframes[i - 1].minute_of_day >= s->timeframes[i].minute_of_day)
    {
      printSerial("Error: Timeframe out of order or overlapping.");
      return false;
    }

    if (s->timeframes[i].lamp_on != 1 && s->timeframes[i].lamp_on != 0)
    {
      printSerial("Error: Lamp Int not 1 or 0");
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

bool save_schedule(Schedule *schedule)
{
  if (schedule == NULL)
  {
    printSerial("Cannot save a null schedule.");
    return false;
  }

  schedule->crc = schedule_crc(schedule);

  return storage_write(SCHEDULE_FILE, schedule, sizeof(*schedule));
}

static bool load_schedule(Schedule *schedule)
{
  if (schedule == NULL)
  {
    printSerial("Cannot load into a null schedule.");
    return false;
  }

  return storage_read(SCHEDULE_FILE, schedule, sizeof(*schedule));
}

bool file_start_up(Schedule *schedule)
{
  if (schedule == NULL)
  {
    printSerial("Cannot load into a null schedule.");
    return false;
  }

  if (!load_schedule(schedule))
  {
    printSerial("Using defaults.");

    schedule_init_default(schedule);
    save_schedule(schedule);

    return false;
  }

  printSerial("Schedule Loaded Successfully.");

  if (!check_crc(schedule))
  {
    printSerial("Invalid CRC.");

    schedule_init_default(schedule);
    save_schedule(schedule);

    return false;
  }

  if (!check_count(schedule))
  {
    printSerial("Invalid count");

    schedule_init_default(schedule);
    save_schedule(schedule);

    return false;
  }

  // Update older versions of save files
  if (schedule->version < 2)
  {
    schedule->version = 2;

    for (uint16_t i = 0; i < schedule->count; i++)
    {
      schedule->timeframes[(int)i].temperature *= 10;
    }

    schedule->crc = schedule_crc(schedule);
  }
  else if (schedule->version > 2)
  {
    printSerial("This version of PucaOS cannot handle that file version.");

    schedule_init_default(schedule);
    save_schedule(schedule);

    return false;
  }

  if (!validate_schedule(schedule))
  {
    printSerial("Invalid schedule loaded; using defaults instead.");

    schedule_init_default(schedule);
    save_schedule(schedule);

    return false;
  }

  save_schedule(schedule);

  return true;
}