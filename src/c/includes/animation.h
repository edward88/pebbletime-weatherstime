#pragma once
#include <pebble.h>

struct animation_info
{
  AnimationCurve animation_type;
  GRect start_position;
  GRect end_position;
  int delay_ms;
  int duration_ms;
  Layer *layer;
};

struct animation_info *animation_custom_setup(struct animation_info *ctx, Layer *layer, AnimationCurve animation_type, int delay_ms, int duration_ms, GRect start_pos, GRect end_pos);
Animation *animation_custom(struct animation_info *ctx);
