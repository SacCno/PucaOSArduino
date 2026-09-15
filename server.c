#include "server.h"
#include "plug.h"
#include "config.h"
#include "utils.h"
#include "logic.h"

#include <stdio.h>
#include <string.h>

KVP topics[] = {
    {"SCH_UPD_MC", "/mc/schedule/update"},
    {"SCH_CHK_MC", "/mc/schedule/check"},
    {"CMD_OFF_MC", "/mc/command/plug"},
    {"CMD_OFF_MC", "/mc/command/mode"},
    {"SCH_UPD_PH", "/ph/schedule/update"},
    {"SCH_CHK_PH", "/ph/schedule/check"},
    {"CMD_OFF_PH", "/ph/command/plug"},
    {"CMD_OFF_PH", "/ph/command/mode"},
};

Schedule *schedule;

static struct mg_mgr *mgr = NULL;

static bool reconnect_scheduled = false;
static bool mqtt_ready = false;

static struct mg_connection *mqtt_connection = NULL;
static struct mg_mqtt_opts mqtt_options;

bool handle_mqtt_msg(struct mg_mqtt_message *message);
static void mqtt_event_handler(struct mg_connection *connection, int event, void *event_data);
static void mqtt_reconnect_timer_fn(void *arg);
static void schedule_mqtt_reconnect(void);

void start_mqtt_client(struct mg_mgr *manager, struct mg_mqtt_opts *options, Schedule *s)
{
  schedule = s;
  mgr = manager;

  mqtt_options = *options;

  mqtt_connection = mg_mqtt_connect(
      mgr,
      MQTT_URL,
      &mqtt_options,
      mqtt_event_handler,
      NULL);

  if (mqtt_connection == NULL)
  {
    printf("Could not make MQTT connection\n");
    schedule_mqtt_reconnect();
    return;
  }
  else
  {
    printf("Connection to %s\n", MQTT_URL);
  }
}

bool mqtt_publish(const char *topic, const char *message)
{
  if (!mqtt_ready || mqtt_connection == NULL)
  {
    printf("Cannot publish: MQTT not connected.\n");
    return false;
  }

  if (topic == NULL || message == NULL)
  {
    return false;
  }

  struct mg_mqtt_opts options;
  options.topic = mg_str(topic);
  options.message = mg_str(message);
  options.qos = 1;
  options.retain = false;

  mg_mqtt_pub(mqtt_connection, &options);

  return true;
}

static void mqtt_event_handler(struct mg_connection *connection, int event, void *event_data)
{

  switch (event)
  {
  case MG_EV_OPEN:
  {
    printf("Network connection opened.\n");
    break;
  }
  case MG_EV_CONNECT:
  {
    struct mg_tls_opts tls_options = {
        .name = mg_url_host(MQTT_URL),
        .skip_verification = true};

    mg_tls_init(connection, &tls_options);
    printf("TLS initialised.\n");
    break;
  }
  case MG_EV_MQTT_OPEN:
  {
    int *connack_status = event_data;
    if (connack_status == NULL || *connack_status != 0)
    {
      printf("MQTT connection rejected, CONNACK code: %d\n",
             connack_status != NULL ? *connack_status : -1);
      mqtt_ready = false;

      break;
    }

    mqtt_ready = true;

    printf("MQTT connection established.\n");

    struct mg_mqtt_opts subscribe_options;
    memset(&subscribe_options, 0, sizeof(subscribe_options));

    for (int i = 4; i < 8; i++) // only subscribe to last four topics
    {
      subscribe_options.topic = mg_str(topics[i].value);
      subscribe_options.qos = 1;

      mg_mqtt_sub(connection, &subscribe_options);

      printf("Subscribed to topic: %s\n", topics[i].value);
    }

    ControlMode cm = get_control_mode();
    infodump(*schedule, cm);

    break;
  }
  case MG_EV_MQTT_MSG:
  {
    struct mg_mqtt_message *message = (struct mg_mqtt_message *)event_data;

    printf("Received MQTT message.\n");

    printf("Topic:\t%.*s\n",
           (int)message->topic.len,
           message->topic.buf);

    printf("Payload:\t%.*s\n",
           (int)message->data.len,
           message->data.buf);

    handle_mqtt_msg(message);

    break;
  }
  case MG_EV_MQTT_CMD:
  {
    struct mg_mqtt_message *message = (struct mg_mqtt_message *)event_data;

    printf("MQTT command received. 0x%02x\n",
           message->cmd);
    break;
  }
  case MG_EV_ERROR:
  {
    printf("Connection error: %s\n",
           event_data != NULL
               ? (char *)event_data
               : "unknown error");
    break;
  }
  case MG_EV_CLOSE:
  {
    if (mqtt_connection == connection)
    {
      mqtt_connection = NULL;
      mqtt_ready = false;
      schedule_mqtt_reconnect();
    }

    printf("Connection closed.\n");
    break;
  }
  default:
    break;
  }
}

bool handle_mqtt_msg(struct mg_mqtt_message *message)
{
  char m[512];
  int n = snprintf(m,
                   sizeof(m),
                   "%.*s",
                   (int)message->data.len,
                   message->data.buf);
  if (n < 0 || (size_t)n >= sizeof(m))
  {
    printf("Error in parsing message. L: %d", n);
    return false;
  }

  char t[32];
  n = snprintf(t,
               sizeof(t),
               "%.*s",
               (int)message->topic.len,
               message->topic.buf);
  if (n < 0 || (size_t)n >= sizeof(t))
  {
    printf("Error in parsing topic.");
    return false;
  }

  if (strcmp(t, "/ph/schedule/update") == 0)
  {
    Schedule new;
    memset(&new, 0, sizeof(new));

    printf("1");

    if (json_to_schedule(&new, message))
    {
      if (new.count == 0)
      {
        new = default_schedule;
      }
      *schedule = new;
      print_schedule(new);
      save_schedule(schedule);

      mqtt_publish("/mc/schedule/update", "valid");
      return true;
    }

    printf("2");

    mqtt_publish("/mc/schedule/update", "invalid");
    return false;
  }
  else if (strcmp(t, "/ph/schedule/check") == 0)
  {
    char message[512];
    schedule_to_json(*schedule, message, sizeof(message));

    mqtt_publish("/mc/schedule/check", message);

    return true;
  }
  else if (strcmp(t, "/ph/command/plug") == 0)
  {
    if (strcmp(m, "on") == 0)
    {
      set_plug(mgr, PLUG_IP, true);
      return true;
    }
    else if (strcmp(m, "off") == 0)
    {
      set_plug(mgr, PLUG_IP, false);
      return true;
    }

    printf("Invalid plug command");
  }
  else if (strcmp(t, "/ph/command/mode") == 0)
  {
    if (strcmp(m, "auto") == 0)
    {
      update_control_mode(CONTROL_MODE_AUTO);
    }
    else if (strcmp(m, "manual") == 0)
    {
      update_control_mode(CONTROL_MODE_MANUAL);
    }
    else
    {
      ControlMode cm = get_control_mode();
      switch (cm)
      {
      case CONTROL_MODE_AUTO:
        mqtt_publish("/mc/command/mode", "auto");
        break;
      case CONTROL_MODE_MANUAL:
        mqtt_publish("/mc/command/mode", "manual");
        break;
      }
    }
  }

  printf("Message Topic Incorrect.");

  return false;
}

static void mqtt_reconnect_timer_fn(void *arg)
{
  (void)arg;

  reconnect_scheduled = false;

  if (mgr == NULL || mqtt_connection != NULL)
  {
    return;
  }

  printf("Attempting MQTT reconnect...\n");

  mqtt_connection = mg_mqtt_connect(
      mgr,
      MQTT_URL,
      &mqtt_options,
      mqtt_event_handler,
      NULL);

  if (mqtt_connection == NULL)
  {
    printf("MQTT reconnect attempt failed.\n");
    reconnect_scheduled = true;

    mg_timer_add(
        mgr,
        1000,
        MG_TIMER_ONCE,
        mqtt_reconnect_timer_fn,
        NULL);
  }
  else
  {
    printf("MQTT reconnect initiated.\n");
  }
}

static void schedule_mqtt_reconnect(void)
{
  if (mgr == NULL || reconnect_scheduled)
  {
    return;
  }
  reconnect_scheduled = true;

  mg_timer_add(
      mgr,
      0,
      MG_TIMER_ONCE,
      mqtt_reconnect_timer_fn,
      NULL);
}