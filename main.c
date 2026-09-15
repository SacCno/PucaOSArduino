#include <stdbool.h>
#include <string.h>

#include "server.h"
#include "schedule.h"
#include "utils.h"
#include "logic.h"
#include "temperature.h"
#include "platform.h"

static struct mg_mgr manager;
static Schedule schedule;

static void logic_timer_callback(void *arg);

static bool device_ready = false;
static bool check_time = false;
static bool check_temp = true;

void pucaos_setup(void)
{
  if (!pucaos_platform_init())
  {
    return;
  }

  char password[32];

  if (!password_reader("/mqttpassword.txt", password, sizeof(password)))
  {
    printSerial("Password not found");
    return;
  }

  struct mg_mqtt_opts mqtt_options;
  memset(&mqtt_options, 0, sizeof(mqtt_options));

  mqtt_options.client_id = mg_str("ESP32-Nano");
  mqtt_options.user = mg_str("ESP32-Nano");
  mqtt_options.pass = mg_str(password);
  mqtt_options.clean = false;
  mqtt_options.keepalive = 60;

  file_start_up(&schedule);
  init_temp();

  mg_mgr_init(&manager);
  logic_init(&schedule, &manager);

  mg_timer_add(
      &manager,
      15000,
      MG_TIMER_REPEAT,
      logic_timer_callback,
      NULL);

  start_mqtt_client(&manager, &mqtt_options, &schedule);

  device_ready = true;
  return;

}


void pucaos_loop(void)
{
  if(device_ready)
  {
    mg_mgr_poll(&manager, 10);
  }
}

static void logic_timer_callback(void *arg)
{
  (void)arg;

  if (check_temp) // tick 1, check temp
  {
    check_temp = false;
    check_time = true;

    update_logic_temp();
  }
  else if (check_time) // tick 2, check time
  {
    check_time = false;

    update_logic_time();
  }
  else // tick three check logic, then repeat
  {
    check_temp = true;

    logic_update();
  }
}