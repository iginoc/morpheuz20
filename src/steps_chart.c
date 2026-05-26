/*
 * Morpheuz Sleep Monitor - Steps Chart
 *
 * Copyright (c) 2013-2016 James Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "pebble.h"
#include "morpheuz.h"
#include "language.h"
#include "rootui.h" // For UiCommon and macro_text_layer_create
#include "steps_chart.h"

static Window *s_steps_chart_window;
static Layer *s_steps_chart_layer;
static TextLayer *s_chart_title_layer;
static bool s_steps_chart_showing = false;

// Shared with rootui, rectui, roundui, primary_window with main and notice_font with noticewindows
extern UiCommon ui;

/*
 * Click handler to close the steps chart window
 */
static void steps_chart_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

/*
 * Click config provider for the steps chart window
 */
static void steps_chart_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, steps_chart_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, steps_chart_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, steps_chart_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, steps_chart_single_click_handler);
}

/*
 * Drawing function for the steps chart bars
 */
static void steps_chart_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t bar_width = (bounds.size.w - (NUM_DAILY_STEP_HISTORY_DAYS + 1) * 2) / NUM_DAILY_STEP_HISTORY_DAYS; // 2px padding per bar
  int16_t max_chart_height = bounds.size.h - 28; // Lascia spazio per etichette e valori in alto

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);

  // Find max steps for scaling
  uint32_t max_steps = 0;
  for (int i = 0; i < NUM_DAILY_STEP_HISTORY_DAYS; i++) {
    if (persist_exists(PERSIST_DAILY_STEPS_0 + i)) {
      uint32_t steps = persist_read_int(PERSIST_DAILY_STEPS_0 + i);
      if (steps > max_steps) {
        max_steps = steps;
      }
    }
  }
  if (max_steps == 0) max_steps = 1; // Avoid division by zero

  // Draw bars and labels
  for (int i = 0; i < NUM_DAILY_STEP_HISTORY_DAYS; i++) {
    uint32_t steps = 0;
    if (persist_exists(PERSIST_DAILY_STEPS_0 + i)) {
      steps = persist_read_int(PERSIST_DAILY_STEPS_0 + i);
    }

    // Calculate bar height (scaled to max_chart_height)
    int16_t bar_height = (int16_t)(((float)steps / max_steps) * max_chart_height);
    if (bar_height == 0 && steps > 0) bar_height = 1; // Ensure tiny bars are visible

    int16_t x_pos = i * (bar_width + 2) + 2; // 2px padding
    int16_t y_pos = bounds.size.h - bar_height - 14; // 14px per l'etichetta della data

    // Draw bar
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(x_pos, y_pos, bar_width, bar_height), 0, GCornerNone);

    // Draw step count value above the bar
    char step_val_buffer[10];
    snprintf(step_val_buffer, sizeof(step_val_buffer), "%lu", steps);
    graphics_draw_text(ctx, step_val_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                       GRect(x_pos - 2, y_pos - 14, bar_width + 4, 14), 
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    
    // Draw date label below the bar
    time_t base_time = persist_exists(PERSIST_DAILY_STEPS_BASE_KEY) ? persist_read_int(PERSIST_DAILY_STEPS_BASE_KEY) : time(NULL);
    struct tm *day_tm = localtime(&base_time);
    day_tm->tm_mday -= i; // Day 0 is yesterday, Day 1 is day before yesterday, etc.
    mktime(day_tm); // Normalize tm struct

    char date_buffer[8]; 
    strftime(date_buffer, sizeof(date_buffer), "%a", day_tm); // Just day name to save space
    graphics_draw_text(ctx, date_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(x_pos, bounds.size.h - 14, bar_width, 14), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  if (max_steps == 0) {
    graphics_draw_text(ctx, "Nessun dato", fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, bounds.size.h/2 - 10, bounds.size.w, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

/*
 * Load handler for the steps chart window
 */
static void steps_chart_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Title Layer
  s_chart_title_layer = macro_text_layer_create(GRect(0, 0, bounds.size.w, 20), window_layer, GColorWhite, GColorBlack, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GTextAlignmentCenter);
  text_layer_set_text(s_chart_title_layer, MENU_STEPS_CHART);

  // Chart Layer
  s_steps_chart_layer = layer_create(GRect(0, 20, bounds.size.w, bounds.size.h - 20));
  layer_set_update_proc(s_steps_chart_layer, steps_chart_update_callback);
  layer_add_child(window_layer, s_steps_chart_layer);

  window_set_click_config_provider(window, steps_chart_click_config_provider);
}

/*
 * Unload handler for the steps chart window
 */
static void steps_chart_window_unload(Window *window) {
  layer_destroy(s_steps_chart_layer);
  text_layer_destroy(s_chart_title_layer);
  window_destroy(window);
  s_steps_chart_showing = false;
}

/*
 * Show the steps chart window
 */
EXTFN void show_steps_chart() {
  if (s_steps_chart_showing) {
    return;
  }
  s_steps_chart_showing = true;

  s_steps_chart_window = window_create();
  window_set_background_color(s_steps_chart_window, GColorBlack);
  window_set_window_handlers(s_steps_chart_window, (WindowHandlers) {
    .load = steps_chart_window_load,
    .unload = steps_chart_window_unload,
  });
  window_stack_push(s_steps_chart_window, true);
}

/*
 * Check if the steps chart is currently showing
 */
EXTFN bool is_steps_chart_showing() {
  return s_steps_chart_showing;
}