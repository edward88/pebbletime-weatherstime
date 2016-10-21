#include <pebble.h>
#include "includes/storage.h"
#include "includes/animation.h"
#include "includes/weather.h"

// declare variables, not war

static Window *s_main_window;

static Layer *s_weather_icon_layer;
static TextLayer *s_weather_desc_layer;
static TextLayer *s_weather_location_layer;
static GBitmap *s_weather_icon_bitmap;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static GColor s_background_color;
static GColor s_band_color;
static GColor s_battery_color;
static GColor s_time_color;
static GColor s_date_color;
static GColor s_weather_color;
static GColor s_location_color;

static bool s_is_animate;
static Animation *s_animation_sequence;

static int s_battery_level;
static Layer *s_battery_top_layer;
static Layer *s_battery_bottom_layer;

static char *s_date_format;

enum temperature_unit {Celsius, Fahrenheit, Kelvin};
static enum temperature_unit s_temperature_unit;

enum status {IDLE, BUSY, UPDATING, OBSTRUCTED};
static enum status s_app_status;

static bool s_app_connected;
static bool s_bt_alert;

static int s_weather_freshness_counter;
static WeatherProvider s_weather_source;

static void set_display_weather_icon(bool hidden)
{
  layer_set_hidden(s_weather_icon_layer, hidden);
}

static void set_display_weather(bool hidden)
{
  set_display_weather_icon(hidden);
  layer_set_hidden(text_layer_get_layer(s_weather_desc_layer), hidden);
  layer_set_hidden(text_layer_get_layer(s_weather_location_layer), hidden);
}

static GRect get_time_layer()
{
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));

  const int time_rect_y = bounds.size.h - 73;
  const int time_round_y = 23;
  const int time_text_h = 55;
  
  return GRect(0, PBL_IF_ROUND_ELSE(time_round_y, time_rect_y), bounds.size.w, time_text_h);
}

static GRect get_date_layer()
{
  GRect bounds = layer_get_bounds(window_get_root_layer(s_main_window));
  const int date_rect_x = bounds.size.w/2 + 4;
  const int date_rect_y = -3;
  const int date_round_x = 0;
  const int date_round_y = 0;
  const int date_text_h = 30;
  return PBL_IF_ROUND_ELSE(GRect(date_round_x, date_round_y, bounds.size.w, date_text_h), GRect(date_rect_x, date_rect_y, bounds.size.w/2 - 9, date_text_h));
}

static GRect get_location_layer()
{
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));
  
  const int location_round_y = 72;
  const int location_rect_y = bounds.size.h - 22;
  const int location_edge_w = 5;
  const int location_h = 30;
  
  return GRect(location_edge_w, PBL_IF_ROUND_ELSE(location_round_y, location_rect_y), bounds.size.w-(2*location_edge_w), location_h);
}

static GRect get_desc_layer()
{
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));
  
  const int desc_round_y = 86;
  const int desc_rect_y = bounds.size.h - 96;
  const int desc_edge_w = 5;
  const int desc_h = 30;
  
  return GRect(desc_edge_w, PBL_IF_ROUND_ELSE(desc_round_y, desc_rect_y), bounds.size.w-(2*desc_edge_w), desc_h);
}

static GRect get_weather_layer()
{
  GRect bounds = layer_get_bounds(window_get_root_layer(s_main_window));
  return PBL_IF_ROUND_ELSE(GRect(0, bounds.size.h/2, bounds.size.w, bounds.size.h/2) , GRect(0, 0, 80, 80));
}

static GRect get_battery_top_layer()
{
  GRect bounds = layer_get_bounds(text_layer_get_layer(s_time_layer));
  return GRect(0, 3, bounds.size.w, 2);
}

static GRect get_battery_bottom_layer()
{
  GRect bounds = layer_get_bounds(text_layer_get_layer(s_time_layer));
  return GRect(0, bounds.size.h - 5, bounds.size.w, 2);
}

static bool is_unobstructed()
{
  Layer *window_layer = window_get_root_layer(s_main_window);
  // Get the full size of the screen
  GRect full_bounds = layer_get_bounds(window_layer);
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(window_layer);
  
  return grect_equal(&full_bounds, &bounds);
}

static void set_unobstructed_change_display()
{
  layer_set_frame(text_layer_get_layer(s_weather_location_layer), get_location_layer());
  layer_set_frame(text_layer_get_layer(s_weather_desc_layer), get_desc_layer());
  layer_set_frame(text_layer_get_layer(s_time_layer), get_time_layer());
}

static void canvas_update_proc(Layer *layer, GContext *ctx)
{
  const int weather_icon_pad = 5;
  GRect bounds = layer_get_bounds(layer);
	graphics_context_set_fill_color(ctx, GColorFromRGB(0, 0, 170));
	
  PBL_IF_ROUND_ELSE(
    graphics_fill_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h-30), 45), 
    graphics_fill_circle(ctx, GPoint(30, 30), 45)
  );
  
  GRect icon_bounds = gbitmap_get_bounds(s_weather_icon_bitmap);
  
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_weather_icon_bitmap, PBL_IF_ROUND_ELSE(GRect(bounds.size.w/2-25, bounds.size.h/2-22, icon_bounds.size.w, icon_bounds.size.h), GRect(weather_icon_pad, weather_icon_pad, icon_bounds.size.w, icon_bounds.size.h)));
}

static void update_time()
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_time_buffer[8];
  static char s_date_buffer[10];
  
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
  
  strftime(s_date_buffer, sizeof(s_date_buffer), s_date_format, tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void update_weather_display()
{
  static char weather_layer_buffer [40];
  static char weather_temperature_buffer[8];
  static char weather_conditions_buffer[32];
  static enum temperature_unit weather_temperature_unit;
  
  if(strcmp(get_last_weather_temperature_unit(), "Celsius") == 0)
  {
    weather_temperature_unit = Celsius;
  }
  else if (strcmp(get_last_weather_temperature_unit(), "Fahrenheit") == 0)
  {
    weather_temperature_unit = Fahrenheit;
  }
  else if (strcmp(get_last_weather_temperature_unit(), "Kelvin") == 0)
  {
    weather_temperature_unit = Kelvin;
  }
  
  snprintf(weather_conditions_buffer, sizeof(weather_conditions_buffer),  "%s", get_last_weather_conditions());
  
  // check if setting remains unchanged
  if(s_temperature_unit == weather_temperature_unit)
  {
    if(s_temperature_unit == Celsius)
    {
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%sC", get_last_weather_temperature_value());
    }
    else if (s_temperature_unit == Fahrenheit)
    {
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%sF", get_last_weather_temperature_value());
    }
    else if (s_temperature_unit == Kelvin)
    {
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%sK", get_last_weather_temperature_value());
    }
  }
  else if (weather_temperature_unit == Celsius)
  {
    if(s_temperature_unit == Fahrenheit)
    {
      // convert celsius to fahrenheit
      float temp_in_F = atoi(get_last_weather_temperature_value()) * 1.8 + 32;
      int d1 = temp_in_F;         // get the integer part
      float f2 = temp_in_F - d1;  // get the fractional part
      int d2 = f2 * 10;           // turn fractional part to int
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%d.%01dF", d1, d2);
    }
    else if (s_temperature_unit == Kelvin)
    {
      // convert celsius to kelvin
      float temp_in_K = atoi(get_last_weather_temperature_value()) + 273.15;
      int d1 = temp_in_K;
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%dK", d1);
    }
  }
  else if (weather_temperature_unit == Fahrenheit)
  {
    if(s_temperature_unit == Celsius)
    {
      //convert from fahrenheit to celsius
      float temp_in_C = (atoi(get_last_weather_temperature_value()) / 1.8) - 32;
      int d1 = temp_in_C;
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%dC", d1);
    }
    else if (s_temperature_unit == Kelvin)
    {
      //convert from fahrenheit to celsius then to kelvin
      float temp_in_K = (atoi(get_last_weather_temperature_value()) / 1.8) - 32 + 273.15;
      int d1 = temp_in_K;
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%dK", d1);
    }
  }
  else if (weather_temperature_unit == Kelvin)
  {
    if(s_temperature_unit == Celsius)
    {
      //convert from kelvin to celsius
      float temp_in_C = (atoi(get_last_weather_temperature_value()) - 273.15);
      int d1 = temp_in_C;
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%dC", d1);
    }
    else if(s_temperature_unit == Fahrenheit)
    {
      //convert kelvin to celsius then to fahrenheit
       float temp_in_F = ((atoi(get_last_weather_temperature_value()) - 273.15) / 1.8) - 32 ;
      int d1 = temp_in_F;
      snprintf(weather_temperature_buffer, sizeof(weather_temperature_buffer), "%dC", d1);
    }
  }
  
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", weather_temperature_buffer, weather_conditions_buffer);
  
  text_layer_set_text(s_weather_desc_layer, weather_layer_buffer);
  text_layer_set_text(s_weather_location_layer, get_last_weather_location());
  bool is_day = get_last_weather_is_day();
  
  switch (get_last_weather_code()) {
    case Clear:
      s_weather_icon_bitmap = is_day ? gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLEAR_DAY) : gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLEAR_NIGHT);
      break;
    case Clouds:
      s_weather_icon_bitmap = is_day ? gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUD_DAY) : gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUD_NIGHT);
      break;
    case Drizzle:
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DRIZZLE);
      break;
    case Rain:
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RAIN);
      break;
    case Thunderstorm:
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THUNDERSTORM);
      break;
    case Snow:
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SNOW);
      break;
    case Atmosphere:
      s_weather_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ATMOSPHERE);
      break;
    default:
      s_weather_icon_bitmap = is_day ? gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUD_DAY) : gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUD_NIGHT);
  }
  
  layer_mark_dirty(s_weather_icon_layer);
}

static void update_weather()
{
  if(s_app_status != UPDATING)
  {
    s_app_status = UPDATING;
    set_last_weather_update();
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_WeatherSource, get_weather_source());
    app_message_outbox_send();
  }
}

static void hide_weather()
{
  GRect bounds;
  struct animation_info *ctx_anim_icon, *ctx_anim_location, *ctx_anim_desc;
  
  bounds = layer_get_bounds(s_weather_icon_layer);
  ctx_anim_icon = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx_anim_icon = animation_custom_setup(ctx_anim_icon, s_weather_icon_layer, AnimationCurveEaseOut, 250, 500, get_weather_layer(), PBL_IF_ROUND_ELSE(GRect(0, bounds.size.h*2, bounds.size.w, bounds.size.h), GRect(-bounds.size.w, -bounds.size.h, bounds.size.w, bounds.size.h)));
  
  s_animation_sequence = animation_custom(ctx_anim_icon);
  
  if(!animation_is_scheduled(s_animation_sequence))
  {
    animation_schedule(s_animation_sequence);
  }
  
  bounds = get_location_layer();
  ctx_anim_location = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx_anim_location = animation_custom_setup(ctx_anim_location, text_layer_get_layer(s_weather_location_layer), AnimationCurveEaseOut, 250, 500, get_location_layer(), GRect(bounds.origin.x+bounds.size.w, bounds.origin.y, bounds.size.w, bounds.size.h));
  
  s_animation_sequence = animation_custom(ctx_anim_location);

  if(!animation_is_scheduled(s_animation_sequence))
  {
    animation_schedule(s_animation_sequence);
  }
  
  bounds = get_desc_layer();
  ctx_anim_desc = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx_anim_desc = animation_custom_setup(ctx_anim_desc, text_layer_get_layer(s_weather_desc_layer), AnimationCurveEaseOut, 250, 500, get_desc_layer(), GRect(bounds.origin.x+bounds.size.w, bounds.origin.y, bounds.size.w, bounds.size.h));
  
  s_animation_sequence = animation_custom(ctx_anim_desc);
  
  if(!animation_is_scheduled(s_animation_sequence))
  {
    animation_schedule(s_animation_sequence);
  }
  
  if(!ctx_anim_icon)
  {
    free(ctx_anim_icon);
  }
  if(!ctx_anim_location)
  {
    free(ctx_anim_location);
  }
  if(!ctx_anim_desc)
  {
    free(ctx_anim_desc);
  }
  
  s_app_status = IDLE;
}

static void display_weather()
{
  GRect bounds;
  struct animation_info *ctx_anim_icon, *ctx_anim_location, *ctx_anim_desc;
  
  bounds = layer_get_bounds(s_weather_icon_layer);
  ctx_anim_icon = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx_anim_icon = animation_custom_setup(ctx_anim_icon, s_weather_icon_layer, AnimationCurveEaseOut, 250, 500, PBL_IF_ROUND_ELSE(GRect(0, bounds.size.h*2, bounds.size.w, bounds.size.h), GRect(-bounds.size.w, -bounds.size.h, bounds.size.w, bounds.size.h)), get_weather_layer());
  
  s_animation_sequence = animation_custom(ctx_anim_icon);

  if(!animation_is_scheduled(s_animation_sequence))
  {
    animation_schedule(s_animation_sequence);
  }
  
  bounds = get_location_layer();
  ctx_anim_location = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx_anim_location = animation_custom_setup(ctx_anim_location, text_layer_get_layer(s_weather_location_layer), AnimationCurveEaseOut, 250, 500, GRect(bounds.origin.x+bounds.size.w, bounds.origin.y, bounds.size.w, bounds.size.h), get_location_layer());
  
  s_animation_sequence = animation_custom(ctx_anim_location);

  if(!animation_is_scheduled(s_animation_sequence))
  {
    animation_schedule(s_animation_sequence);
  }
  
  bounds = get_desc_layer();
  ctx_anim_desc = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx_anim_desc = animation_custom_setup(ctx_anim_desc, text_layer_get_layer(s_weather_desc_layer), AnimationCurveEaseOut, 250, 500, GRect(bounds.origin.x+bounds.size.w, bounds.origin.y, bounds.size.w, bounds.size.h), get_desc_layer());
  
  s_animation_sequence = animation_custom(ctx_anim_desc);
  
  if(!animation_is_scheduled(s_animation_sequence))
  {
    animation_schedule(s_animation_sequence);
  }
  
  if(!ctx_anim_icon)
  {
    free(ctx_anim_icon);
  }
  if(!ctx_anim_location)
  {
    free(ctx_anim_location);
  }
  if(!ctx_anim_desc)
  {
    free(ctx_anim_desc);
  }
  
  // wait to hide weather info
  uint32_t timeout_ms = 2000;
  app_timer_register(timeout_ms, hide_weather, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
  
  if(s_weather_freshness_counter < 30)
  {
    s_weather_freshness_counter++;
  }
  else if (s_app_connected)
  {
    update_weather();
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction)
{
  if(axis == ACCEL_AXIS_Y)
  {
    if(s_app_status == IDLE)
    {
      s_app_status = BUSY;
      display_weather();
    }
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{
  Tuple *jsready_tuple = dict_find(iterator, MESSAGE_KEY_JSREADY);
  
  if(jsready_tuple)
  {
    if(jsready_tuple->value->int32 == 1)
    {
      if(!is_weather_fresh() && s_app_connected)
      {
        update_weather();
      }
      else
      {
        update_weather_display();
      }
      return;
    }
  }
  
  // Read tuples for weather data
  Tuple *temperature_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  Tuple *code_tuple = dict_find(iterator, MESSAGE_KEY_CODE);
  Tuple *description_tuple = dict_find(iterator, MESSAGE_KEY_DESCRIPTION);
  Tuple *location_tuple = dict_find(iterator, MESSAGE_KEY_LOCATION);
  Tuple *is_day_tuple = dict_find(iterator, MESSAGE_KEY_ISDAY);
  
  if(location_tuple)
  {
    char location_layer_buffer[64];
    
    snprintf(location_layer_buffer, sizeof(location_layer_buffer), "%s", location_tuple->value->cstring);
    set_last_weather_location(location_layer_buffer);
  }
  
  // If all weather data is available, use it
  if(temperature_tuple && conditions_tuple && code_tuple && description_tuple && is_day_tuple)
  {
    // Store incoming information
    char temperature_buffer[8];
    char conditions_buffer[32];
    bool is_day_buffer;
  
    if(s_temperature_unit == Celsius)
    {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)temperature_tuple->value->int32);
      set_last_weather_temperature_value(temperature_buffer);
      set_last_weather_temperature_unit("Celsius");
    }
    else if(s_temperature_unit == Fahrenheit)
    {
      // note: the following is required as snprintf does not support floating point
      // convert celsius to fahrenheit
      float temp_in_F = temperature_tuple->value->int32 * 1.8 + 32;
      int d1 = temp_in_F;         // get the integer part
      float f2 = temp_in_F - d1;  // get the fractional part
      int d2 = f2 * 10;           // turn fractional part to int
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d.%01d", d1, d2);
      set_last_weather_temperature_value(temperature_buffer);
      set_last_weather_temperature_unit("Fahrenheit");
    }
    else if (s_temperature_unit == Kelvin)
    {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)(temperature_tuple->value->int32 + 273.15));
      set_last_weather_temperature_value(temperature_buffer);
      set_last_weather_temperature_unit("Kelvin");
    }
    
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    set_last_weather_conditions(conditions_buffer);
    
    is_day_buffer = is_day_tuple->value->int32 == 1;
    set_last_weather_is_day(is_day_buffer);
    
    set_last_weather_code((int)code_tuple->value->int32);
    
    s_weather_freshness_counter = 0;
    
    s_app_status = IDLE;
    
    update_weather_display();
  }
  
  Tuple *background_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  if(background_color_t)
  {
    s_background_color = GColorFromHEX(background_color_t->value->int32);
    set_background_colour(background_color_t->value->int32);
    window_set_background_color(s_main_window, s_background_color);
  }

  Tuple *band_color_t = dict_find(iterator, MESSAGE_KEY_TimeBandColor);
  if(band_color_t)
  {
    s_band_color = GColorFromHEX(band_color_t->value->int32);
    set_band_colour(band_color_t->value->int32);
    text_layer_set_background_color(s_time_layer, s_band_color);
  }

  Tuple *battery_color_t = dict_find(iterator, MESSAGE_KEY_BatteryColor);
  if(battery_color_t)
  {
    s_battery_color = GColorFromHEX(battery_color_t->value->int32);
    set_battery_colour(battery_color_t->value->int32);
    layer_mark_dirty(s_battery_top_layer);
    layer_mark_dirty(s_battery_bottom_layer);
  }
  
  Tuple *time_color_t = dict_find(iterator, MESSAGE_KEY_TimeColor);
  if(time_color_t)
  {
    s_time_color = GColorFromHEX(time_color_t->value->int32);
    set_time_colour(time_color_t->value->int32);
    text_layer_set_text_color(s_time_layer, s_time_color);
  }
  
  Tuple *date_color_t = dict_find(iterator, MESSAGE_KEY_DateColor);
  if(date_color_t)
  {
    s_date_color = GColorFromHEX(date_color_t->value->int32);
    set_date_colour(date_color_t->value->int32);
    text_layer_set_text_color(s_date_layer, s_date_color);
  }
  
  Tuple *weather_color_t = dict_find(iterator, MESSAGE_KEY_WeatherColor);
  if(weather_color_t)
  {
    s_weather_color = GColorFromHEX(weather_color_t->value->int32);
    set_weather_colour(weather_color_t->value->int32);
    text_layer_set_text_color(s_weather_desc_layer, s_weather_color);
  }
  
  Tuple *location_color_t = dict_find(iterator, MESSAGE_KEY_LocationColor);
  if(location_color_t)
  {
    s_location_color = GColorFromHEX(location_color_t->value->int32);
    set_location_colour(location_color_t->value->int32);
    text_layer_set_text_color(s_weather_location_layer, s_location_color);
  }
  
  Tuple *animate_t = dict_find(iterator, MESSAGE_KEY_Animate);
  if(animate_t)
  {
    s_is_animate = animate_t->value->int32 == 1;
    set_animate(s_is_animate);
    
    set_display_weather(s_is_animate);
    
    if(s_is_animate)
    {
      // subscribe to tap events
      accel_tap_service_subscribe(accel_tap_handler);
    }
    else
    {
      // unsubscribe tap events
      accel_tap_service_unsubscribe();
      
      // set layers back to original positons
      layer_set_frame(text_layer_get_layer(s_weather_desc_layer), get_desc_layer());
      layer_set_frame(text_layer_get_layer(s_weather_location_layer), get_location_layer());
      layer_set_frame(s_weather_icon_layer, get_weather_layer());
    }
  }
  
  Tuple *date_format_t = dict_find(iterator, MESSAGE_KEY_DateFormat);
  if(date_format_t)
  {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%s", date_format_t->value->cstring);
    
    if(strcmp(s_date_format, buffer) != 0)
    {
      set_date_format(date_format_t->value->cstring);
      s_date_format = get_date_format();
      update_time();
    }
  }
  
  Tuple *temperature_unit_t = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if(temperature_unit_t)
  {
    enum temperature_unit buffer = s_temperature_unit;
    if(strcmp(temperature_unit_t->value->cstring, "Celsius") == 0)
    {
      s_temperature_unit = Celsius;
    }
    else if(strcmp(temperature_unit_t->value->cstring, "Fahrenheit") == 0)
    {
      s_temperature_unit = Fahrenheit;
    }
    else if(strcmp(temperature_unit_t->value->cstring, "Kelvin") == 0)
    {
      s_temperature_unit = Kelvin;
    }
    
    if(s_temperature_unit != buffer)
    {
      set_temperature_unit(temperature_unit_t->value->cstring);
      update_weather_display();
    }
  }
  
  Tuple *weather_source_t = dict_find(iterator, MESSAGE_KEY_WeatherSource);
  if(weather_source_t)
  {
    if(atoi(weather_source_t->value->cstring) != (int)s_weather_source)
    {
      set_weather_source(atoi(weather_source_t->value->cstring));
      s_weather_source = get_weather_source();
      if(s_app_connected)
      {
        update_weather();
      }
    }
  }
  
  Tuple *bt_alert_t = dict_find(iterator, MESSAGE_KEY_BluetoothAlert);
  if(bt_alert_t)
  {
    s_bt_alert = bt_alert_t->value->int32 == 1;
    set_bt_alert(s_bt_alert);
  }
}


static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
  //
  s_app_status = IDLE;
  if(s_weather_freshness_counter == 0)
  {
    update_weather_display();
  }
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  //
  s_app_status = IDLE;
  if(s_weather_freshness_counter == 0)
  {
    update_weather_display();
  }
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  //
  s_app_status = IDLE;
  if(s_weather_freshness_counter == 0)
  {
    update_weather_display();
  }
}

static void battery_update_proc(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  
  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);

  // Draw the bar
  graphics_context_set_fill_color(ctx, s_battery_color);
  graphics_fill_rect(ctx, GRect((bounds.size.w-width)/2, 0, width, bounds.size.h), 0, GCornerNone);
}

static void battery_callback(BatteryChargeState state)
{
  s_battery_level = state.charge_percent;
}

static void main_window_load(Window *window)
{
  //get user's locale based on pebble phone app settings
  setlocale(LC_ALL, "");
  
  s_app_status = is_unobstructed() ? IDLE : OBSTRUCTED;
  
  Layer *window_layer = window_get_root_layer(window);
  
  s_weather_freshness_counter = 0;
  
	s_weather_icon_layer = layer_create(get_weather_layer());
	layer_set_update_proc(s_weather_icon_layer, canvas_update_proc);
	s_time_layer = text_layer_create(get_time_layer());
  s_date_layer = text_layer_create(get_date_layer());
  s_weather_desc_layer = text_layer_create(get_desc_layer());
  s_weather_location_layer = text_layer_create(get_location_layer());
  
  // set the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, s_band_color);
  text_layer_set_text_color(s_time_layer, s_time_color);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, s_date_color);
  text_layer_set_font(s_date_layer, PBL_IF_ROUND_ELSE(fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD) ,fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD)));
  text_layer_set_text_alignment(s_date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
  
  text_layer_set_background_color(s_weather_desc_layer, GColorClear);
  text_layer_set_text_color(s_weather_desc_layer, s_weather_color);
  text_layer_set_text_alignment(s_weather_desc_layer, GTextAlignmentRight);
  text_layer_set_font(s_weather_desc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  
  text_layer_set_background_color(s_weather_location_layer, GColorClear);
  text_layer_set_text_color(s_weather_location_layer, s_location_color);
  text_layer_set_text_alignment(s_weather_location_layer, GTextAlignmentRight);
  text_layer_set_font(s_weather_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  
  // Create battery meter Layer
  s_battery_top_layer = layer_create(get_battery_top_layer());
  s_battery_bottom_layer = layer_create(get_battery_bottom_layer());

  layer_set_update_proc(s_battery_top_layer, battery_update_proc);
  layer_set_update_proc(s_battery_bottom_layer, battery_update_proc);
  
  layer_add_child(text_layer_get_layer(s_time_layer), s_battery_top_layer);
  layer_add_child(text_layer_get_layer(s_time_layer), s_battery_bottom_layer);
  
	layer_add_child(window_layer, s_weather_icon_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_desc_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_location_layer));
  
  if(s_is_animate)
  {
    set_display_weather(s_is_animate);
  }
  
  if(s_app_status == OBSTRUCTED)
  {
    set_display_weather_icon(true);
  }
  
  if(!s_app_connected)
  {
    update_weather_display();
  }
}

static void main_window_unload(Window *window)
{
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_desc_layer);
  text_layer_destroy(s_weather_location_layer);
  
  // Destroy Layers
  layer_destroy(s_weather_icon_layer);
  layer_destroy(s_battery_top_layer);
  layer_destroy(s_battery_bottom_layer);
  
  // Destroy GBitmap
  gbitmap_destroy(s_weather_icon_bitmap);
}

static void unobstructed_will_change(GRect final_unobstructed_screen_area, void *context)
{
  // Get the full size of the screen
  GRect full_bounds = layer_get_bounds(window_get_root_layer(s_main_window));
  if (!grect_equal(&full_bounds, &final_unobstructed_screen_area)) {
    // Screen is about to become obstructed, hide the weather info
    set_display_weather_icon(true);
    s_app_status = OBSTRUCTED;
  }
}

static void unobstructed_change(AnimationProgress progress, void *context)
{
  set_unobstructed_change_display();
}

static void unobstructed_did_change(void *context) {
  if (is_unobstructed()) {
    // Screen is no longer obstructed, immediately show the weather icon when animation is turned off
    set_display_weather_icon(s_is_animate);
    set_unobstructed_change_display();
    s_app_status = IDLE;
  }
}

static void app_connection_handler(bool connected)
{
  s_app_connected = connected;
  if(s_bt_alert)
  {
    if(s_app_connected)
    {
      vibes_double_pulse();
    }
    else
    {
      vibes_short_pulse();
    }
  }
}

static void init()
{
  // initialize storage
  init_storage();
  
  // get colors and settings
  s_background_color = GColorFromHEX(get_background_colour());
  s_band_color = GColorFromHEX(get_band_colour());
  s_battery_color = GColorFromHEX(get_battery_colour());
  s_time_color = GColorFromHEX(get_time_colour());
  s_date_color = GColorFromHEX(get_date_colour());
  s_weather_color = GColorFromHEX(get_weather_colour());
  s_location_color = GColorFromHEX(get_location_colour());
  s_is_animate = get_animate();
  s_temperature_unit = strcmp(get_temperature_unit(), "Celsius") == 0 ? Celsius : Fahrenheit;
  s_date_format = get_date_format();
  s_weather_source = (WeatherProvider)get_weather_source();
  
  s_bt_alert = get_bt_alert();
  
  // Peek at the Pebble app connections
  s_app_connected = connection_service_peek_pebble_app_connection();
  
  // Create main Window element
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Set main background colour
  window_set_background_color(s_main_window, s_background_color);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  if(s_is_animate)
  {
    // subscribe to tap events
    accel_tap_service_subscribe(accel_tap_handler);
  }
  
  UnobstructedAreaHandlers handlers = {
    .will_change = unobstructed_will_change,
    .change = unobstructed_change,
    .did_change = unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
  
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = app_connection_handler
  });
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  const bool animated = true;
  window_stack_push(s_main_window, animated);
}

static void deinit()
{
  // Destroy Window
  window_destroy(s_main_window);
  if(s_is_animate)
  {
    // unsubscribe tap events
    accel_tap_service_unsubscribe();
  }
  
  // unsubscribe unobstructed area events
  unobstructed_area_service_unsubscribe();
  
  // unsubscribe connection service
  connection_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
