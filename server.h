#ifndef SERVER_H
#define SERVER_H

#include "mongoose.h"
#include "schedule.h"

bool mqtt_publish(const char *topic, const char *message, bool request);
void start_mqtt_client(struct mg_mgr *manager, struct mg_mqtt_opts *options, Schedule *s);

#endif