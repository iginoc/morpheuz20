/*
 * Morpheuz Sleep Monitor
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
#include "rootui.h"

#ifdef PBL_RECT

static TextLayer *s_title_layer; // Declare a static TextLayer for the title
// Private
// Shared with rootui, rectui, roundui, primary_window with main and notice_font with noticewindows
extern UiCommon ui;

/*
 * Version is no longer external, as it's not used in this simplified UI.
 */

/*
 * Load on window load
 */
EXTFN void morpheuz_load(Window *window) {
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);

  // Titolo statico
  s_title_layer = macro_text_layer_create(GRect(0, 30, 144, 30), window_layer, GColorWhite, GColorClear, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GTextAlignmentCenter);
  text_layer_set_text(s_title_layer, "OGGI");
  
  // Layer per il conteggio passi
  // ui.time_font is already loaded here.
  ui.notice_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_16)); // Initialize notice_font
  ui.time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_38)); // Load font here, as it's used for steps
  ui.steps_layer = macro_text_layer_create(GRect(0, 65, 144, 50), window_layer, GColorWhite, GColorClear, ui.time_font, GTextAlignmentCenter);

  // Nuovo layer per il conteggio del sonno
  ui.sleep_layer = macro_text_layer_create(GRect(0, 115, 144, 30), window_layer, GColorWhite, GColorClear, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter); // Use system font for sleep

  ui.animation_count = 6; // Segnala che la UI è pronta e non ci sono animazioni pendenti

  // Initialize other UI elements used by rootui.c but not explicitly drawn in rectui.c
  // These will be hidden or placed off-screen.
  ui.powernap_layer = macro_text_layer_create(GRect(0, 0, 1, 1), window_layer, GColorWhite, GColorClear, fonts_get_system_font(FONT_KEY_GOTHIC_14), GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer_jf(ui.powernap_layer), true);

  ui.text_date_smart_alarm_range_layer = macro_text_layer_create(GRect(0, 0, 1, 1), window_layer, GColorWhite, GColorClear, fonts_get_system_font(FONT_KEY_GOTHIC_24), GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer_jf(ui.text_date_smart_alarm_range_layer), true);

  init_icon_cache(); // Ensure icon cache is initialized before icon_bar
  ui.icon_bar = macro_layer_create(GRect(0, 0, 1, 1), window_layer, &icon_bar_update_callback);
  layer_set_hidden(ui.icon_bar, true);

  ui.progress_layer = macro_layer_create(GRect(0, 0, 1, 1), window_layer, &progress_layer_update_callback);
  layer_set_hidden(ui.progress_layer, true);

  macro_bitmap_layer_create(&ui.alarm_button_top, GRect(0, 0, 1, 1), window_layer, RESOURCE_ID_BUTTON_ALARM_TOP, false);
  macro_bitmap_layer_create(&ui.alarm_button_button, GRect(0, 0, 1, 1), window_layer, RESOURCE_ID_BUTTON_ALARM_BOTTOM, false);

  morpheuz_load_standard_postamble();

  app_timer_register(250, post_init_hook, NULL);
}

/*
 * Shutdown
 */
EXTFN void morpheuz_unload(Window *window) {
  text_layer_destroy(s_title_layer); // Destroy the title layer
  text_layer_destroy(ui.steps_layer);
  fonts_unload_custom_font(ui.time_font);
  fonts_unload_custom_font(ui.notice_font); // Unload notice_font
  text_layer_destroy(ui.sleep_layer);
  text_layer_destroy(ui.powernap_layer);
  text_layer_destroy(ui.text_date_smart_alarm_range_layer);
  layer_destroy(ui.icon_bar);
  layer_destroy(ui.progress_layer);
  macro_bitmap_layer_destroy(&ui.alarm_button_top);
  macro_bitmap_layer_destroy(&ui.alarm_button_button);
}

/*
 * Hide the bed when making the analogue face visible
 */
EXTFN void bed_visible(bool value) {
  // Stub: componenti grafici rimossi nella versione semplificata
}

#endif
