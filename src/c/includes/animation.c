#include "animation.h"

static void anim_started_handler(Animation *animation, void *context) {

  struct animation_info *ctx;
  ctx = (struct animation_info*)malloc(sizeof(struct animation_info));
  ctx = (struct animation_info *)context;
  
  layer_set_hidden(ctx->layer, false);
  
  if(!ctx) {
    free(ctx);
  }
}

struct animation_info *animation_custom_setup(struct animation_info *ctx, Layer *layer, AnimationCurve animation_type, int delay_ms, int duration_ms, GRect start_pos, GRect end_pos)
{
  ctx->layer = layer;
  ctx->animation_type = animation_type;
  ctx->delay_ms = delay_ms;
  ctx->duration_ms = duration_ms;
  ctx->start_position = start_pos;
  ctx->end_position = end_pos;

  return ctx;
}

Animation *animation_custom(struct animation_info *ctx)
{
  Layer *layer = ctx->layer;

  // the start and end frames  
  GRect start = ctx->start_position;
  GRect finish = ctx->end_position;
  
  // animate the layer
  PropertyAnimation *prop_anim = property_animation_create_layer_frame(layer, &start, &finish);
  
  // get the animation
  Animation *anim = property_animation_get_animation(prop_anim);

  // configure the animation's curve, delay, and duration
  animation_set_curve(anim, ctx->animation_type);
  animation_set_delay(anim, ctx->delay_ms);
  animation_set_duration(anim, ctx->duration_ms);


  // set some handlers
  animation_set_handlers(anim, (AnimationHandlers) {
    .started = anim_started_handler
  }, ctx);

  return anim;
}

