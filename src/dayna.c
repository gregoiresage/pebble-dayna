#include "pebble.h"

static Window *window;
static Layer *hours_layer;
static Layer *minutes_layer;
static Layer *seconds_layer;

static uint8_t hours    = 0;
static uint8_t minutes  = 0;
static uint8_t seconds  = 0;
static bool bluetooth_connected = false;

#define HOUR_TICKNESS 5
#define MINUTE_TICKNESS 7
#define MINUTE_GAP 2
#define MINUTE_VERTICAL_GAP 13

static GPath *hours_path;
static const GPathInfo HOURS_PATH_POINTS = {
  5,
  (GPoint []) {
    {0, 15},
    {MINUTE_TICKNESS, 15},
    {55, 84},
    {MINUTE_TICKNESS, 168-15},
    {0, 168-15},
  }
};

void minutes_layer_update_callback(Layer *layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  uint8_t leftxpos = bounds.size.w / 2 + (5 * MINUTE_TICKNESS + 4 * MINUTE_GAP) / 2;
  uint8_t bottomypos = bounds.size.h / 2 + (4 * (3 * MINUTE_TICKNESS + 2 * MINUTE_GAP) + 3 * MINUTE_VERTICAL_GAP) / 2;

  uint8_t xpos = leftxpos - MINUTE_TICKNESS;
  uint8_t ypos = bottomypos + MINUTE_VERTICAL_GAP + 1;

  // minutes = 59;

  for(uint8_t minute=1; minute <= 60; minute++){
    
    if((minute - 1) % 5 == 0){
      xpos = leftxpos - MINUTE_TICKNESS;
    }
    else {
      xpos -= MINUTE_TICKNESS + MINUTE_GAP;
    }

    if((minute - 1) % 15 == 0){
      ypos -= MINUTE_TICKNESS + MINUTE_VERTICAL_GAP;
    }
    else if((minute - 1) % 5 == 0){
      ypos -= MINUTE_TICKNESS + MINUTE_GAP;
    }

    if(minute <= minutes){
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, GRect(xpos, ypos, MINUTE_TICKNESS, MINUTE_TICKNESS), 0, GCornerNone);
    }
    else {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_draw_pixel(ctx, GPoint(xpos + MINUTE_TICKNESS/2, ypos + MINUTE_TICKNESS/2));
    }
  }
}

void hours_layer_update_callback(Layer *layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);

  // hours = 23;
  uint8_t tmpHours = hours % 12 != 0 ? hours % 12 : 12;

  graphics_context_set_fill_color(ctx, GColorWhite);
  for(uint8_t hour=1; hour <= tmpHours; hour++){
    graphics_fill_rect(ctx, GRect(2, 17 + (hour-1) * (168 - 17*2) / 11 - HOUR_TICKNESS/2, bounds.size.w - 14, HOUR_TICKNESS), 0, GCornerNone);
  }

  gpath_move_to(hours_path, GPoint(MINUTE_TICKNESS,0));
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, hours_path);

  gpath_move_to(hours_path, GPoint(0,0));
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, hours_path);

  if(hours >= 12){
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(30, bounds.size.h / 2 - 5, 10, 10), 0, GCornerNone);
  }
}

void seconds_layer_update_callback(Layer *layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);

  if(seconds % 2 == 0){
    GPoint p1 = GPoint(bounds.size.w/2, 5);
    GPoint p2 = GPoint(bounds.size.w/2, bounds.size.h - 5);
    if(bluetooth_connected){
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_draw_line(ctx, p1, p2);
    }
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(p1.x - 3, p1.y - 3, 7, 7), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(p2.x - 3, p2.y - 3, 7, 7), 0, GCornerNone);
  }
}

void handle_seconds_tick(struct tm *tick_time, TimeUnits units_changed) {
  if(hours != tick_time->tm_hour){
    hours = tick_time->tm_hour;
    layer_mark_dirty(hours_layer);
  }
  if(minutes != tick_time->tm_min){
    minutes = tick_time->tm_min;
    layer_mark_dirty(minutes_layer);
  }
  if(seconds != tick_time->tm_sec){
    seconds = tick_time->tm_sec;
    layer_mark_dirty(seconds_layer);
  }
}

void bluetooth_connection_handler(bool connected){
  if(connected != bluetooth_connected){
    bluetooth_connected = connected;
    layer_mark_dirty(seconds_layer);
  }
}

void handle_deinit(void) {
  gpath_destroy(hours_path);
  layer_destroy(hours_layer);
  layer_destroy(minutes_layer);
  layer_destroy(seconds_layer);
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
}

void handle_init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);

  GRect hours_frame = GRect(144/2 - 16, 0, 144/2 + 16, 168);
  hours_layer = layer_create(hours_frame);
  layer_set_update_proc(hours_layer, hours_layer_update_callback);
  layer_add_child(window_layer, hours_layer);

  GRect minutes_frame = GRect(0, 0, 144/2, 168);
  minutes_layer = layer_create(minutes_frame);
  layer_set_update_proc(minutes_layer, minutes_layer_update_callback);
  layer_add_child(window_layer, minutes_layer);

  GRect seconds_frame = GRect((144-11) / 2, (168 - 40)/2, 11, 40);
  seconds_layer = layer_create(seconds_frame);
  layer_set_update_proc(seconds_layer, seconds_layer_update_callback);
  layer_add_child(window_layer, seconds_layer);

  hours_path = gpath_create(&HOURS_PATH_POINTS);

  time_t now;
  time(&now);
  tick_timer_service_subscribe(SECOND_UNIT, handle_seconds_tick);
  handle_seconds_tick(localtime(&now), SECOND_UNIT);

  bluetooth_connection_service_subscribe(bluetooth_connection_handler);
  bluetooth_connected = bluetooth_connection_service_peek();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}