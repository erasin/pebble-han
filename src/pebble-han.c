#include <pebble.h>
#include <stdio.h>


#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS  1

static Window *window;
static TextLayer *time_text_layer, *date_text_layer , *bluetooth_text_layer, *battery_text_layer ;

char timeBuffer[8];
char dateBuffer[12];
char batteryBuffer[4];

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
  // 时间
  strftime(timeBuffer, sizeof(timeBuffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(time_text_layer, timeBuffer);

  // 日期: 年月日 周几
  strftime(dateBuffer, sizeof(dateBuffer), "%y.%m.%d.%u", tick_time);
  text_layer_set_text(date_text_layer, dateBuffer);
}

// 蓝牙信息
void handle_bluetooth(bool connected){
  if(connected == 0){
    text_layer_set_text(bluetooth_text_layer, "\U0001F636");
  }
  if(connected == 1){
    text_layer_set_text(bluetooth_text_layer, "\U0001F603");
  }
}

// 电池信息
void handle_battery(BatteryChargeState charge_state) {
  snprintf(batteryBuffer, sizeof(batteryBuffer), "%dB", charge_state.charge_percent);
  // 电量变色
  if (charge_state.charge_percent >= 85) {
    text_layer_set_text_color(battery_text_layer, GColorGreen);
  } else if (charge_state.charge_percent <= 30) {
    text_layer_set_text_color(battery_text_layer, GColorRed );
  } else {
    text_layer_set_text_color(battery_text_layer, GColorBlue);
  }
  text_layer_set_text(battery_text_layer, batteryBuffer);
}


// 窗口加载
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 文本层
  time_text_layer = textLayerInit(GRect(0, 45, bounds.size.w, 50), GColorWhite, GColorBlack, FONT_KEY_LECO_36_BOLD_NUMBERS, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

  date_text_layer = textLayerInit(GRect(0, 95, bounds.size.w, 30),  GColorWhite, GColorBlack, FONT_KEY_LECO_20_BOLD_NUMBERS, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_text_layer));

  battery_text_layer = textLayerInit(GRect(15, 0, 30, 20),  GColorWhite, GColorBlack, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));

  bluetooth_text_layer = textLayerInit(GRect(2, 0, 13, 20), GColorWhite, GColorBlack, FONT_KEY_GOTHIC_18, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(bluetooth_text_layer));

  // 时间
  time_t timestamp = time(NULL);
  struct tm *time = localtime(&timestamp);
  handle_minute_tick(time, MINUTE_UNIT);

  // 电池信息获取
  BatteryChargeState btchg = battery_state_service_peek();
  handle_battery(btchg);

  // 蓝牙信息获取
  bool connected = bluetooth_connection_service_peek();
  handle_bluetooth(connected);

}

// 释放资源
static void window_unload(Window *window) {
  text_layer_destroy(time_text_layer);
  text_layer_destroy(date_text_layer);
  text_layer_destroy(battery_text_layer);
  text_layer_destroy(bluetooth_text_layer);
}

// 初始事件
static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);

  // 安分执行
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
}

// 释放事件
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
