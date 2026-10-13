#ifndef SETTINGS_H
#define SETTINGS_H

#include "storage.h"
#include "logic.h"

struct settings_type
{
  ControlMode mode;
};

bool settings_init();
bool update_settings_mode(ControlMode cm);

#endif