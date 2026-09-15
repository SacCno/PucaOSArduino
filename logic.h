#ifndef LOGIC_H
#define LOGIC_H

#include <stdint.h>
#include "schedule.h"
#include "mongoose.h"

typedef enum
{
  CONTROL_MODE_AUTO,
  CONTROL_MODE_MANUAL
} ControlMode;

void logic_init(Schedule *schedule, struct mg_mgr *manager);
void logic_update();
void plug_state_received(struct mg_http_message event_data);
void update_logic_temp();
void update_logic_time();
void update_control_mode(ControlMode mode);
ControlMode get_control_mode();

#endif