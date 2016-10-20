#include "storage.h"

// increment the version number if schema changes
const int current_storage_version = 5;

const uint32_t storage_version_key = 0;

const uint32_t background_colour_key = 1;
const uint32_t band_colour_key = 2;
const uint32_t time_colour_key = 3;
const uint32_t date_colour_key = 4;
const uint32_t weather_colour_key = 5;
const uint32_t location_colour_key = 6;
const uint32_t battery_colour_key = 7;

const uint32_t temperature_unit_key = 8;
const uint32_t animate_key = 9;

const uint32_t last_weather_update_key = 10;
const uint32_t last_weather_code_key = 11;
const uint32_t last_weather_conditions_key = 12;
const uint32_t last_weather_location_key = 13;
const uint32_t last_weather_is_day_key = 14;

const uint32_t last_weather_temperature_value_key = 15;
const uint32_t last_weather_temperature_unit_key = 16;

const uint32_t date_format_key = 17;

const uint32_t bt_alert_key = 18;

const uint32_t weather_source_key = 20;

void init_storage()
{
  if (persist_exists(storage_version_key))
  {
    // Check the last storage scheme version the app used
    int last_storage_version = persist_read_int(storage_version_key);
    if(last_storage_version == current_storage_version)
    {
      return;
    }
    else
    {
      //purge save data when upgrading from storage version < 4
      int keys;
      for (keys = 1; keys <= 17 ; keys++)
      {
        if(persist_exists(keys))
        {
          persist_delete(keys);
        }
      }
    }
  }
  // Store the current storage scheme version number
  persist_write_int(storage_version_key, current_storage_version);
}

void set_last_weather_update()
{
  time_t temp = time(NULL);
  localtime(&temp);
  persist_write_data(last_weather_update_key, &temp, sizeof(temp));
}

time_t  get_last_weather_update()
{
  time_t last_weather_update;
  persist_read_data(last_weather_update_key, &last_weather_update, sizeof(last_weather_update));
  return last_weather_update;
}

bool is_weather_fresh()
{
  time_t current_time = time(NULL);
  localtime(&current_time);
  time_t last_weather_update = get_last_weather_update();

  // check if any previous updates
  if(!last_weather_update)
  {
    return false;
  }

  double diff = difftime(current_time, last_weather_update);

  return diff < 60 * 30; // 30mins
}

void set_last_weather_code(int value)
{
  persist_write_int(last_weather_code_key, value);
}

int get_last_weather_code()
{
  // set default code if not set
  if (!persist_exists(last_weather_code_key))
  {
    set_last_weather_code(-1);
  }
  return persist_read_int(last_weather_code_key);
}


void set_last_weather_temperature_value(char *value)
{
  persist_write_string(last_weather_temperature_value_key, value);
}

char *get_last_weather_temperature_value()
{
  if (!persist_exists(last_weather_temperature_value_key))
  {
    set_last_weather_temperature_value("");
  }
  static char buffer[8];
  persist_read_string(last_weather_temperature_value_key, buffer, sizeof(buffer));
  return buffer;
}

void set_last_weather_temperature_unit(char *value)
{
  persist_write_string(last_weather_temperature_unit_key, value);
}

char *get_last_weather_temperature_unit()
{
  // set default weather description if not set
  if (!persist_exists(last_weather_temperature_unit_key))
  {
    set_last_weather_temperature_unit("Celsius");
  }
  static char buffer[12];
  persist_read_string(last_weather_temperature_unit_key, buffer, sizeof(buffer));
  return buffer;
}

void set_last_weather_conditions(char *value)
{
  persist_write_string(last_weather_conditions_key, value);
}

char *get_last_weather_conditions()
{
  // set default weather description if not set
  if (!persist_exists(last_weather_conditions_key))
  {
    set_last_weather_conditions("");
  }
  static char buffer[40];
  persist_read_string(last_weather_conditions_key, buffer, sizeof(buffer));
  return buffer;
}

void set_last_weather_location(char *value)
{
  persist_write_string(last_weather_location_key, value);
}

char *get_last_weather_location() {
  // set default weather location if not set
  if (!persist_exists(last_weather_location_key))
  {
    set_last_weather_location("");
  }
  static char buffer[64];
  persist_read_string(last_weather_location_key, buffer, sizeof(buffer));
  return buffer;
}

void set_last_weather_is_day(bool value)
{
  persist_write_bool(last_weather_is_day_key, value);
}

bool get_last_weather_is_day() {
  // set default animation if not set
  if (!persist_exists(last_weather_is_day_key))
  {
    set_last_weather_is_day(true);
  }
  return persist_read_bool(last_weather_is_day_key);
}

void set_background_colour(int value)
{
  persist_write_int(background_colour_key, value);
}

int get_background_colour()
{
  // set default colour if not set
  if (!persist_exists(background_colour_key))
  {
    set_background_colour(0x000000);
  }
  return persist_read_int(background_colour_key);
}

void set_band_colour(int value)
{
  persist_write_int(band_colour_key, value);
}

int get_band_colour()
{
  // set default colour if not set
  if (!persist_exists(band_colour_key))
  {
    set_band_colour(0xFF0000);
  }
  return persist_read_int(band_colour_key);
}

void set_battery_colour(int value)
{
  persist_write_int(battery_colour_key, value);
}

int get_battery_colour()
{
  // set default colour if not set
  if (!persist_exists(battery_colour_key))
  {
    set_battery_colour(0xFFFFFF);
  }
  return persist_read_int(battery_colour_key);
}

void set_time_colour(int value)
{
  persist_write_int(time_colour_key, value);
}

int get_time_colour()
{
  // set default colour if not set
  if (!persist_exists(time_colour_key))
  {
    set_time_colour(0xFFFFFF);
  }
  return persist_read_int(time_colour_key);
}

void set_date_colour(int value)
{
  persist_write_int(date_colour_key, value);
}

int get_date_colour()
{
  // set default colour if not set
  if (!persist_exists(date_colour_key))
  {
    set_date_colour(0xFFFFFF);
  }
  return persist_read_int(date_colour_key);
}

void set_weather_colour(int value)
{
  persist_write_int(weather_colour_key, value);
}

int get_weather_colour()
{
  // set default colour if not set
  if (!persist_exists(weather_colour_key))
  {
    set_weather_colour(0xFFFFFF);
  }
  return persist_read_int(weather_colour_key);
}

void set_location_colour(int value)
{
  persist_write_int(location_colour_key, value);
}

int get_location_colour()
{
  // set default colour if not set
  if (!persist_exists(location_colour_key))
  {
    set_location_colour(0xFFFFFF);
  }
  return persist_read_int(location_colour_key);
}

void set_date_format(char *value)
{
  persist_write_string(date_format_key, value);
}

char *get_date_format()
{
  // set default date format if not set
  if (!persist_exists(date_format_key))
  {
    set_date_format("%d %b");
  }
  static char buffer[10];
  persist_read_string(date_format_key, buffer, sizeof(buffer));
  return buffer;
}


void set_temperature_unit(char *value)
{
  persist_write_string(temperature_unit_key, value);
}

char *get_temperature_unit()
{
  // set default temperature unit if not set
  if (!persist_exists(temperature_unit_key))
  {
    set_temperature_unit("Celsius");
  }
  static char buffer[32];
  persist_read_string(temperature_unit_key, buffer, sizeof(buffer));
  return buffer;
}

void set_animate(bool value)
{
  persist_write_bool(animate_key, value);
}

bool get_animate()
{
  // set default animation if not set
  if (!persist_exists(animate_key))
  {
    set_animate(false);
  }
  return persist_read_bool(animate_key);
}

void set_weather_source(int value)
{
  persist_write_int(weather_source_key, value);
}
int get_weather_source()
{
  // set default weather sourse if not set
  if (!persist_exists(weather_source_key))
  {
    set_weather_source(0);
  }
  return persist_read_int(weather_source_key);
}

void set_bt_alert(bool value)
{
  persist_write_bool(bt_alert_key, value);
}

bool get_bt_alert()
{
  // set default animation if not set
  if (!persist_exists(bt_alert_key))
  {
    set_bt_alert(false);
  }
  return persist_read_bool(bt_alert_key);
}
