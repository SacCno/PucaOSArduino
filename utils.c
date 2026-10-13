#include "utils.h"
#include "server.h"
#include "platform.h"
#include <time.h>

uint16_t current_minute_of_day(void)
{
  time_t now = time(NULL);
  struct tm local_time;

#if defined(_WIN32)
  localtime_s(&local_time, &now);
#else
  localtime_r(&now, &local_time);
#endif

  return (uint16_t)(local_time.tm_hour * 60) + local_time.tm_min;
}

bool append_string(char *dest, size_t dest_size, size_t *pos, const char *src);

bool depc_password_reader(
    const char *src,
    char *output,
    size_t output_size)
{
  if (src == NULL || output == NULL || output_size == 0)
  {
    return false;
  }

  FILE *file = fopen(src, "r");
  if (file == NULL)
  {
    perror("Could not open MQTT password file");
    return false;
  }

  if (fgets(output, output_size, file) == NULL)
  {
    fclose(file);
    fprintf(stderr, "Could not read MQTT password file\n");
    return false;
  }

  fclose(file);

  output[strcspn(output, "\r\n")] = '\0';

  if (output[0] == '\0')
  {
    fprintf(stderr, "MQTT password file is empty\n");
    return false;
  }

  return true;
}

uint16_t get_current_timeframe_index(Schedule s, uint16_t current_minute)
{

  int active = -1;

  for (uint16_t i = 0; i < s.count; i++)
  {
    if (s.timeframes[i].minute_of_day <= current_minute)
    {
      active = i;
    }
    else
    {
      break;
    }
  }

  // wraps around midnight
  if (active < 0 && s.count > 0)
  {
    active = s.count - 1;
  }

  return active >= 0 ? active : 0;
}

bool json_to_schedule(Schedule *s, struct mg_mqtt_message *msg)
{
  if (s == NULL || msg == NULL)
  {
    return false;
  }

  Schedule new = {0};

  double count;
  Timeframe tfs[MAX_TIMEFRAMES];
  if (mg_json_get_num(msg->data, "$.count", &count) <= 0)
  {
    return false;
  }
  if (count < 0 || count > MAX_TIMEFRAMES)
  {
    return false;
  }

  int n = -1;
  for (int i = 0; i < (int)count; i++)
  {
    double temperature;
    double minute;
    double lamp_on;

    char path[64];
    n = snprintf(
        path,
        sizeof(path),
        "$.timeframes[%d].temperature",
        (int)i);
    if (n < 0 || (size_t)n >= sizeof(path))
    {
      return false;
    }
    if (mg_json_get_num(msg->data, path, &temperature) <= 0)
    {
      return false;
    }

    n = snprintf(
        path,
        sizeof(path),
        "$.timeframes[%d].minute_of_day",
        (int)i);
    if (n < 0 || (size_t)n >= sizeof(path))
    {
      return false;
    }
    if (mg_json_get_num(msg->data, path, &minute) <= 0)
    {
      return false;
    }

    n = snprintf(
        path,
        sizeof(path),
        "$.timeframes[%d].lamp_on",
        (int)i);
    if (n < 0 || (size_t)n >= sizeof(path))
    {
      return false;
    }
    if (mg_json_get_num(msg->data, path, &lamp_on) <= 0)
    {
      return false;
    }

    tfs[(int)i].temperature = (uint16_t)temperature;
    tfs[(int)i].minute_of_day = (uint16_t)minute;
    tfs[(int)i].lamp_on = (uint16_t)lamp_on;
  }

  if (!schedule_init(&new, 2, tfs, (uint16_t)count))
  {
    return false;
  }

  if (!validate_schedule(&new))
  {
    return false;
  }

  *s = new;
  return true;
}

bool schedule_to_json(Schedule s, char *output, size_t output_size)
{
  if (output == NULL)
  {
    return false;
  }
  if (!validate_schedule(&s))
  {
    return false;
  }

  char opening[48];
  char closing[] = "\t]\n}";

  int n = snprintf(
      opening,
      sizeof(opening),
      "{\n\t\"count\": %d,\n\t\"timeframes\": [\n",
      (int)s.count);

  if (n < 0 || (size_t)n >= sizeof(opening))
  {
    printSerial("Schedule to JSON: String empty due to faileure.");
    return false;
  }

  size_t pos = 0;

  if (!append_string(output, output_size, &pos, opening))
  {
    return false;
  }

  for (uint16_t i = 0; i < s.count; i++)
  {
    char tf[128];

    if (i < s.count - 1)
    {
      n = snprintf(
          tf,
          sizeof(tf),
          "\t\t{\n\t\t\t\"minute_of_day\": %d,\n\t\t\t\"temperature\": %d,\n\t\t\t\"lamp_on\": %d\n\t\t},\n",
          (int)s.timeframes[i].minute_of_day,
          (int)s.timeframes[i].temperature,
          (int)s.timeframes[i].lamp_on);
    }
    else
    {
      n = snprintf(
          tf,
          sizeof(tf),
          "\t\t{\n\t\t\t\"minute_of_day\": %d,\n\t\t\t\"temperature\": %d,\n\t\t\t\"lamp_on\": %d\n\t\t}\n",
          (int)s.timeframes[i].minute_of_day,
          (int)s.timeframes[i].temperature,
          (int)s.timeframes[i].lamp_on); // no final comma
    }

    if (n < 0 || (size_t)n >= sizeof(tf))
    {
      printSerial("Error creating JSON");
      return false;
    }

    if (!append_string(output, output_size, &pos, tf))
    {
      return false;
    }
  }

  if (!append_string(output, output_size, &pos, closing))
  {
    return false;
  }
  return true;
}

bool append_string(char *dest, size_t dest_size, size_t *pos, const char *src)
{
  // Get the existing length of the source text
  size_t len = strlen(src);

  // Check if the size of the source and existing dest fit in dest
  if (*pos + len + 1 > dest_size)
  {
    printSerial("Append String Failure: destination size too small.");
    return false;
  }

  // Append src to the end of dest, adjust the position variable, and add null termintator
  memcpy(dest + *pos, src, len);
  *pos += len;
  dest[*pos] = '\0';

  return true;
}

void print_schedule(Schedule s)
{
  for (int i = 0; i < s.count; i++)
  {
    printSerial("Count: %d | Temperature: %d | Minute: %d | Lamp On: %d",
                (int)s.count,
                (int)s.timeframes[i].temperature,
                (int)s.timeframes[i].minute_of_day,
                (int)s.timeframes[i].lamp_on);
  }
}

void infodump(Schedule s, ControlMode cm)
{
  char json[512];
  schedule_to_json(s, json, sizeof(json));

  size_t pos = strlen(json);

  switch (cm)
  {
  case CONTROL_MODE_AUTO:
  {
    char mode[] = "\n\nauto";
    append_string(json, sizeof(json), &pos, mode);
    break;
  }
  case CONTROL_MODE_MANUAL:
  {
    char mode[] = "\n\nmanual";
    append_string(json, sizeof(json), &pos, mode);
  }
  }

  mqtt_publish("/mc/startup", json, false);
  return;
}