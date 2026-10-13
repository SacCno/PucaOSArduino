#include <stdio.h>
#include <stdlib.h>

#include "mongoose.h"
#include "plug.h"
#include "config.h"
#include "logic.h"
#include "platform.h"

struct plug_request
{
  char host[64];
  char path[128];
  bool query_state;
};

static void plug_http_handler(struct mg_connection *connection, int event, void *event_data);

bool get_plug_state(struct mg_mgr *manager)
{
  if (manager == NULL)
  {
    return false;
  }

  struct plug_request *request = calloc(1, sizeof(*request));

  if (request == NULL)
  {
    return false;
  }

  snprintf(
      request->host,
      sizeof(request->host),
      "%s",
      PLUG_IP);
  snprintf(
      request->path,
      sizeof(request->path),
      "%s",
      "/cm?cmnd=power1");
  request->query_state = true;

  char url[256];
  int n = snprintf(
      url,
      sizeof(url),
      "http://%s",
      PLUG_IP);

  if (n < 0 || (size_t)n >= sizeof(url))
  {
    free(request);
    return false;
  }

  struct mg_connection *connection = mg_http_connect(
      manager,
      url,
      plug_http_handler,
      request);

  if (connection == NULL)
  {
    free(request);
    return false;
  }

  return true;
}

static void plug_http_handler(struct mg_connection *connection, int event, void *event_data)
{
  switch (event)
  {
  case MG_EV_CONNECT:
  {
    struct plug_request *request = (struct plug_request *)connection->fn_data;

    mg_printf(connection,
              "GET %s HTTP/1.1\r\n"
              "Host: %s\r\n"
              "Connection: close\r\n\r\n",
              request->path,
              request->host);
    break;
  }
  case MG_EV_HTTP_MSG:
  {
    struct mg_http_message *message =
        (struct mg_http_message *)event_data;

    struct plug_request *response = connection->fn_data;
    if (response->query_state)
    {
      plug_state_received(*message);
    }
    else
    {
      printSerial("Plug HTTP response: %.*s",
                  (int)message->body.len,
                  message->body.buf);
    }

    connection->is_draining = 1;
    break;
  }
  case MG_EV_ERROR:
    printSerial("Plug HTTP error: %s",
                event_data != NULL
                    ? (char *)event_data
                    : "unknown error");
    break;
  case MG_EV_CLOSE:
    free(connection->fn_data);
    printSerial("Plug HTTP connection closed");
    break;
  }
}

bool set_plug(struct mg_mgr *manager, char *ip, bool turn_on)
{
  if (manager == NULL)
  {
    printSerial("Manager not initialised.");
    return false;
  }

  struct plug_request *request =
      calloc(1, sizeof(*request));

  if (request == NULL)
  {
    return false;
  }

  snprintf(request->host,
           sizeof(request->host),
           "%s",
           ip);

  snprintf(request->path,
           sizeof(request->path),
           "/cm?cmnd=power1%%20%s",
           turn_on ? "on" : "off");

  char url[64];

  snprintf(url,
           sizeof(url),
           "http://%s",
           ip);

  struct mg_connection *connection = mg_http_connect(manager,
                                                     url,
                                                     plug_http_handler,
                                                     request);

  if (connection == NULL)
  {
    free(request);
    printSerial("Could not create HTTP request for plug %s", ip);
    return false;
  }

  printSerial("Sent request to set plug %s: %s",
              ip,
              turn_on ? "on" : "off");

  return true;
}