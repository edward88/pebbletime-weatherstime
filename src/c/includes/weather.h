#pragma once
#include <pebble.h>

//! Possible weather providers
typedef enum {
  OpenWeatherMap      = 0,
  WeatherUnderground  = 1,
  DarkSky             = 2,
  YahooWeather        = 3,
} WeatherProvider;

//! Possible weather conditions
typedef enum {
  Clear         = 0,
  Clouds        = 1,
  Drizzle       = 2,
  Rain          = 3,
  Thunderstorm  = 4,
  Snow          = 5,
  Atmosphere    = 6,
  Unknown       = 99
} WeatherConditionCode;

struct weather_info
{
  WeatherProvider Provider;
  WeatherConditionCode ConditionCode;
};
