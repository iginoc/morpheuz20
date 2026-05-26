#include "pebble.h"
#include "morpheuz.h"
#include "language.h"
#include "rootui.h"

static Window *s_sleep_chart_window;
static Layer *s_sleep_chart_layer;
static TextLayer *s_chart_title_layer;
static bool s_sleep_chart_showing = false;

extern UiCommon ui;

static void sleep_chart_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

static void sleep_chart_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, sleep_chart_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, sleep_chart_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, sleep_chart_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, sleep_chart_single_click_handler);
}

static void sleep_chart_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t bar_width = (bounds.size.w - 16) / 7;
  int16_t max_h = bounds.size.h - 20;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);

  uint32_t max_total = 0;
  for (int i = 0; i < 7; i++) {
    uint32_t total = persist_read_int(PERSIST_DAILY_LIGHT_0 + i) + persist_read_int(PERSIST_DAILY_DEEP_0 + i);
    if (total > max_total) max_total = total;
  }
  if (max_total == 0) max_total = 1;

  time_t base_time = persist_exists(PERSIST_DAILY_STEPS_BASE_KEY) ? persist_read_int(PERSIST_DAILY_STEPS_BASE_KEY) : time(NULL);

  for (int i = 0; i < 7; i++) {
    uint32_t light = persist_read_int(PERSIST_DAILY_LIGHT_0 + i);
    uint32_t deep = persist_read_int(PERSIST_DAILY_DEEP_0 + i);

    int16_t deep_h = (int16_t)((deep * max_h) / max_total);
    int16_t light_h = (int16_t)((light * max_h) / max_total);

    int16_t x = i * (bar_width + 2) + 2;
    int16_t y_base = bounds.size.h - 14;

    // Deep sleep (Solid white)
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(x, y_base - deep_h, bar_width, deep_h), 0, GCornerNone);

    // Light sleep (Outlined/Dotted pattern - we use rectangle with small gap for B&W)
    for (int py = 0; py < light_h; py += 2) {
       graphics_draw_line(ctx, GPoint(x, y_base - deep_h - py), GPoint(x + bar_width - 1, y_base - deep_h - py));
    }

    // Labels
    struct tm *day_tm = localtime(&base_time);
    day_tm->tm_mday -= i;
    mktime(day_tm);
    char buffer[4];
    strftime(buffer, sizeof(buffer), "%a", day_tm);
    graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(x, y_base, bar_width, 14), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  if (max_total == 1) {
    graphics_draw_text(ctx, "Nessun dato", fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, bounds.size.h/2 - 10, bounds.size.w, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void sleep_chart_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_chart_title_layer = macro_text_layer_create(GRect(0, 0, bounds.size.w, 20), window_layer, GColorWhite, GColorBlack, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GTextAlignmentCenter);
  text_layer_set_text(s_chart_title_layer, MENU_SLEEP_CHART);
  s_sleep_chart_layer = layer_create(GRect(0, 20, bounds.size.w, bounds.size.h - 20));
  layer_set_update_proc(s_sleep_chart_layer, sleep_chart_update_callback);
  layer_add_child(window_layer, s_sleep_chart_layer);
  window_set_click_config_provider(window, sleep_chart_click_config_provider);
}

static void sleep_chart_window_unload(Window *window) {
  layer_destroy(s_sleep_chart_layer);
  text_layer_destroy(s_chart_title_layer);
  window_destroy(window);
  s_sleep_chart_showing = false;
}

void show_sleep_chart() {
  if (s_sleep_chart_showing) return;
  s_sleep_chart_showing = true;
  s_sleep_chart_window = window_create();
  window_set_background_color(s_sleep_chart_window, GColorBlack);
  window_set_window_handlers(s_sleep_chart_window, (WindowHandlers) { .load = sleep_chart_window_load, .unload = sleep_chart_window_unload });
  window_stack_push(s_sleep_chart_window, true);
}

bool is_sleep_chart_showing() { return s_sleep_chart_showing; }