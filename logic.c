#include "logic.h"
#include "plug.h"
#include "config.h"
#include "utils.h"
#include "temperature.h"

ControlMode control_mode = CONTROL_MODE_AUTO;

Schedule *current_schedule;
uint16_t current_temp;
uint16_t current_time;
bool lamp_on = false;
static struct mg_mgr *mgr = NULL;

static void apply_logic();

void update_control_mode(ControlMode mode)
{
  control_mode = mode;
}

void plug_state_received(struct mg_http_message event_data)
{
  char *output = mg_json_get_str(event_data.body, "$.POWER");
  if (strcmp(output, "ON") == 0)
  {
    lamp_on = true;
  }
  else if (strcmp(output, "OFF") == 0)
  {
    lamp_on = false;
  }
  else
  {
    lamp_on = NULL;
    printf("Invalid lamp response.");
  }

  apply_logic();
}

void logic_init(Schedule *schedule, struct mg_mgr *manager)
{
  current_schedule = schedule;
  mgr = manager;
  update_logic_temp();
  update_logic_time();
  get_plug_state(mgr);
}

void update_logic_time()
{
  current_time = current_minute_of_day();
}

void update_logic_temp()
{
  current_temp = sensor_temp();
}

void logic_update()
{
  get_plug_state(mgr);
}

static void apply_logic()
{
  if (control_mode == CONTROL_MODE_MANUAL)
  {
    return;
  }

  uint16_t tf_index = get_current_timeframe_index(*current_schedule, current_time);
  Timeframe current_t = current_schedule->timeframes[tf_index];

  // if we're well above temp or schedule says turn off, and the lamp is already on
  if ((current_t.temperature + 2 < current_temp || !current_t.lamp_on) && lamp_on) // 2 degree range
  {
    set_plug(mgr, PLUG_IP, false); // turn off
    printf("\nTurning off lamp.\n\n");
  } // if the lamps off, temps below target, and lamp should be on
  else if (!lamp_on && current_t.temperature - 3 > current_temp && current_t.lamp_on) // 3 degree under range
  {
    set_plug(mgr, PLUG_IP, true);
    printf("\nTurning on lamp.\n\n");
  }

  printf("Target Temp: %f\nCurrent Temp: %f\nLamp Status: %s\n",
         (float)current_schedule->timeframes[tf_index].temperature / 10.0,
         (float)current_temp / 10.0,
         lamp_on ? "On" : "Off");
}

ControlMode get_control_mode()
{
  return control_mode;
}