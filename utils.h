#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "schedule.h"
#include "logic.h"
#include "mongoose.h"

typedef struct
{
    char *key;
    char *value;
} KVP;

uint16_t get_current_timeframe_index(Schedule s, uint16_t current_minute);
bool password_reader(
    const char *src,
    char *output,
    size_t output_size);
bool schedule_to_json(Schedule s, char *output, size_t output_size);
bool json_to_schedule(Schedule *s, struct mg_mqtt_message *msg);
void print_schedule(Schedule s);
void infodump(Schedule s, ControlMode cm);
uint16_t current_minute_of_day(void);

#endif