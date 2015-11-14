#include <pebble.h>
#include <stdio.h>


#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS  1

static Window *window;
static TextLayer *time_text_layer, *date_text_layer , *bluetooth_text_layer, *battery_text_layer ;

char timeBuffer[] = "00:00";
char dateBuffer[] = "00年00月00日";

// 字体渲染
static TextLayer* textLayerInit(GRect location, GColor colour, GColor background, const char *res_id, GTextAlignment alignment)
{
  TextLayer *layer = text_layer_create(location);
  text_layer_set_text_color(layer, colour);
  text_layer_set_background_color(layer, background);
  // fonts_load_custom_font()
  text_layer_set_font(layer, fonts_get_system_font(res_id));
  text_layer_set_text_alignment(layer, alignment);

  return layer;
}

// 时间更新
static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed)
{

  strftime(timeBuffer, sizeof(timeBuffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(time_text_layer, timeBuffer);

  strftime(dateBuffer, sizeof(dateBuffer), "%y.%m.%d", tick_time);
  text_layer_set_text(date_text_layer, dateBuffer);
  //itoa(time->tm_min)
  
  // 每30分钟更新天气信息
  if(tick_time->tm_min % 30 == 0) {
     // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
 
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
 
    // Send the message!
    app_message_outbox_send();
  }
}

// 蓝牙信息
void handle_bluetooth(bool connected){
  if(connected == 0){
    text_layer_set_text(bluetooth_text_layer, "=");
  }
  if(connected == 1){
    text_layer_set_text(bluetooth_text_layer, "&");
  }
}

// 电池信息
void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "000";
  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(battery_text_layer, battery_text);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  time_text_layer = textLayerInit(GRect(0, 45, bounds.size.w, 50), GColorWhite, GColorBlack, FONT_KEY_LECO_36_BOLD_NUMBERS, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

  date_text_layer = textLayerInit(GRect(0, 95, bounds.size.w, 30),  GColorWhite, GColorBlack, FONT_KEY_LECO_20_BOLD_NUMBERS, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_text_layer));

  battery_text_layer = textLayerInit(GRect(50, 0, 20, 20),  GColorWhite, GColorBlack, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));

  bluetooth_text_layer = textLayerInit(GRect(0, 0, 20, 20), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(bluetooth_text_layer));

  // 时间
  time_t timestamp = time(NULL);
  struct tm *time = localtime(&timestamp);
  handle_minute_tick(time, MINUTE_UNIT);

  // 电池
  BatteryChargeState btchg = battery_state_service_peek();
  handle_battery(btchg);

  // 蓝牙
  bool connected = bluetooth_connection_service_peek();
  handle_bluetooth(connected);

}

static void window_unload(Window *window) {
  text_layer_destroy(time_text_layer);
  text_layer_destroy(date_text_layer);
  text_layer_destroy(battery_text_layer);
  text_layer_destroy(bluetooth_text_layer);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
