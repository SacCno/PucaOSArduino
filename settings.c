#include "settings.h"

#define SETTINGS_PATH "/settings.bin"

struct settings_type settings = {0};

static bool load_settings()
{
  return storage_read(SETTINGS_PATH, &settings, sizeof(settings));
}

static bool save_settings()
{
  return storage_write(SETTINGS_PATH, &settings, sizeof(settings));
}

bool update_settings_mode(ControlMode cm)
{
  if (cm != CONTROL_MODE_AUTO && cm != CONTROL_MODE_MANUAL)
  {
    return false;
  }

  settings.mode = cm;
  return save_settings();
}

bool settings_init()
{
  if (!load_settings())
  {
    settings.mode = CONTROL_MODE_AUTO;
    return save_settings();
  }
  else if (settings.mode != CONTROL_MODE_AUTO && settings.mode != CONTROL_MODE_MANUAL)
  {
    settings.mode = CONTROL_MODE_AUTO;
    return save_settings();
  }

  update_logic_mode(settings.mode);

  return true;
}
