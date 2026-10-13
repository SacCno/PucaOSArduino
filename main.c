#include <stdbool.h>
#include <string.h>

#include "server.h"
#include "schedule.h"
#include "utils.h"
#include "logic.h"
#include "temperature.h"
#include "platform.h"
#include "config.h"
#include "display.h"
#include "settings.h"

static struct mg_mgr manager;
static Schedule schedule;

static void logic_timer_callback(void *arg);

static bool device_ready = false;

static char password[32];

void pucaos_setup(void)
{
  if (!pucaos_platform_init())
  {
    return;
  }

  if (!password_reader("/mqttpassword.txt", password, sizeof(password)))
  {
    printSerial("Password not found");
    return;
  }

  if (settings_init())
  {
    printSerial("Settings Loaded Successfully");
  }

  struct mg_mqtt_opts mqtt_options;
  memset(&mqtt_options, 0, sizeof(mqtt_options));

  mqtt_options.client_id = mg_str("ArduinoNanoESP32");
  mqtt_options.user = mg_str(MQTT_USERNAME);
  mqtt_options.pass = mg_str(password);
  mqtt_options.clean = false;
  mqtt_options.keepalive = 60;

  file_start_up(&schedule);
  init_temp();
  displayInit();

  mg_mgr_init(&manager);
  logic_init(&schedule, &manager);

  mg_timer_add(
      &manager,
      5000,
      MG_TIMER_REPEAT,
      logic_timer_callback,
      NULL);

  start_mqtt_client(&manager, &mqtt_options, &schedule);

  device_ready = true;
  return;
}

void pucaos_loop(void)
{
  if (device_ready)
  {
    mg_mgr_poll(&manager, 10);
  }
}

static void logic_timer_callback(void *arg)
{
  (void)arg;

  update_logic_temp();

  update_logic_time();

  logic_update();
}