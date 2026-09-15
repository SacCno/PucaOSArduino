#ifndef PLUG_H
#define PLUG_H

#include <stdbool.h>

typedef void (*plug_state_callback)(
    bool is_on,
    void *arg);

struct mg_mgr;

bool get_plug_state(struct mg_mgr *manager);
bool set_plug(struct mg_mgr *manager, char *ip, bool turn_on);

#endif