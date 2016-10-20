#pragma once
#include <pebble.h>

void init_storage();
void set_last_weather_update();
time_t  get_last_weather_update();
bool is_weather_fresh();

void set_last_weather_code(int value);
int get_last_weather_code();
void set_last_weather_temperature_value(char *value);
char *get_last_weather_temperature_value();
void set_last_weather_temperature_unit(char *value);
char *get_last_weather_temperature_unit();
void set_last_weather_conditions(char *value);
char *get_last_weather_conditions();
void set_last_weather_location(char *value);
char *get_last_weather_location();
void set_last_weather_is_day(bool value);
bool get_last_weather_is_day();

void set_background_colour(int value);
int get_background_colour();
void set_band_colour(int value);
int get_band_colour();
void set_battery_colour(int value);
int get_battery_colour();
void set_time_colour(int value);
int get_time_colour();
void set_date_colour(int value);
int get_date_colour();
void set_weather_colour(int value);
int get_weather_colour();
void set_location_colour(int value);
int get_location_colour();

void set_date_format(char *value);
char *get_date_format();

void set_temperature_unit(char *value);
char *get_temperature_unit();

void set_animate(bool value);
bool get_animate();

void set_weather_source(int value);
int get_weather_source();

void set_bt_alert(bool value);
bool get_bt_alert();
