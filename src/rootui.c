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
#include "steps_chart.h" // Include the new steps chart header
#include "rootui.h"
 
// Private  

// Shared with rootui, rectui, roundui, primary_window with main and notice_font with noticewindows
UiCommon ui;

static AppTimer *ha_timer = NULL;

// Shared with menu, rootui and presets 
char date_text[DATE_FORMAT_LEN] = "";

static bool icon_state[MAX_ICON_STATE];
static int previous_mday = -1;

/*
 * Recupera il numero di passi ufficiale dal sistema Health di Pebble
 * o dal persist storage per Aplite.
 */
static uint32_t get_current_steps() {
  if (persist_exists(PERSIST_STEPS_KEY)) {
    return persist_read_int(PERSIST_STEPS_KEY);
  }
  return get_internal_data()->steps; // Fallback per altre piattaforme o se non c'è gestione specifica
}

/*
 * Esegue il reset dei passi e del sonno al cambio giorno rilevato (alla prima connessione BT)
 */
static void perform_daily_reset() {
  // Archivia la cronologia dei passi prima di resettare
  uint32_t current_steps = get_current_steps();
  for (int i = NUM_DAILY_STEP_HISTORY_DAYS - 1; i > 0; i--) {
    persist_write_int(PERSIST_DAILY_STEPS_0 + i, persist_read_int(PERSIST_DAILY_STEPS_0 + i - 1));
  }
  persist_write_int(PERSIST_DAILY_STEPS_0, current_steps);
  persist_write_int(PERSIST_DAILY_STEPS_BASE_KEY, time(NULL) - 86400); // Riferimento a "ieri"

  // Archivia i dati del sonno accumulati finora nella cronologia 7 giorni
  archive_daily_data();

  // Reset dei passi
  persist_write_int(PERSIST_STEPS_KEY, 0);
  get_internal_data()->steps = 0;
  persist_write_bool(PERSIST_GOAL_MET_KEY, false);

  // Reset della sessione di sonno per il nuovo giorno
  // Questo permette all'attivazione automatica di ripartire se il BT è ancora spento
  InternalData *id = get_internal_data();
  id->has_been_reset = false;
  id->highest_entry = 0;
  set_icon(false, IS_RECORD);
  set_icon(false, IS_IGNORE);

  LOG_INFO("Reset giornaliero passi completato");
}

// Fallback per il Resource ID se non ancora aggiunto nel pannello risorse/appinfo.json
#ifndef RESOURCE_ID_NOTICE_STEP_GOAL_MET
#define RESOURCE_ID_NOTICE_STEP_GOAL_MET RESOURCE_ID_NOTICE_TIMER_RESET_NOALARM
#endif

static void ha_timer_handler(void *data) {
  // Invia esplicitamente i passi correnti al telefono
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint32(iter, KEY_STEPS, get_current_steps());
    app_message_outbox_send();
  }

  ha_timer = app_timer_register(5000, ha_timer_handler, NULL);
}

/*
 * Aggiorna il conteggio dei passi a video
 */
static void update_steps_display() {
  uint32_t steps = get_current_steps();
  static char steps_buffer[16];
  snprintf(steps_buffer, sizeof(steps_buffer), "%lu", steps);
  
  // Controlla se l'obiettivo passi è stato raggiunto e mostra la notifica
  if (steps >= STEP_GOAL) {
    if (!persist_exists(PERSIST_GOAL_MET_KEY) || !persist_read_bool(PERSIST_GOAL_MET_KEY)) {
      show_notice(RESOURCE_ID_NOTICE_STEP_GOAL_MET);
      persist_write_bool(PERSIST_GOAL_MET_KEY, true); // Marca l'obiettivo come raggiunto per oggi
    }
  }
  text_layer_set_text(ui.steps_layer, steps_buffer);
}

/*
 * Aggiorna il conteggio del sonno a video
 */
static void update_sleep_display() {
  static char sleep_buffer[48];
  uint32_t last_light = persist_exists(PERSIST_DAILY_LIGHT_0) ? (uint32_t)persist_read_int(PERSIST_DAILY_LIGHT_0) : 0;
  uint32_t last_deep = persist_exists(PERSIST_DAILY_DEEP_0) ? (uint32_t)persist_read_int(PERSIST_DAILY_DEEP_0) : 0;
  uint32_t last_total = last_light + last_deep;

  if (is_monitoring_sleep()) {
    InternalData *id = get_internal_data();
    int sleep_minutes = 0;
    int i;

    // Calcoliamo il sonno attuale: ogni segmento da 10 minuti con movimento < LIGHT_ABOVE
    // è considerato tempo di sonno.
    for (i = 0; i <= id->highest_entry; i++) {
      if (id->points[i] < LIGHT_ABOVE && !id->ignore[i]) {
        sleep_minutes += 10;
      }
    }
    snprintf(sleep_buffer, sizeof(sleep_buffer), "In corso: %dh %02dm", sleep_minutes / 60, sleep_minutes % 60);
  } else {
    snprintf(sleep_buffer, sizeof(sleep_buffer), "Ult: %dh%02dm (P:%dh)", 
             (int)last_total / 60, (int)last_total % 60, (int)last_deep / 60);
  }
  text_layer_set_text(ui.sleep_layer, sleep_buffer);
}

static Window *s_reset_steps_window;
static MenuLayer *s_reset_steps_menu;

static uint16_t reset_steps_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 2;
}

static void reset_steps_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    case 0: menu_cell_basic_draw(ctx, cell_layer, MENU_CANCEL, NULL, NULL); break;
    case 1: menu_cell_basic_draw(ctx, cell_layer, MENU_RESET_STEPS_CONFIRM, NULL, NULL); break;
  }
}

static int16_t reset_steps_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void reset_steps_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  graphics_context_set_text_color(ctx, MENU_HEAD_COLOR);
  graphics_draw_text(ctx, MENU_RESET_STEPS, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, -2, layer_get_bounds(cell_layer).size.w, 32), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void reset_steps_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row == 1) {
    persist_write_int(PERSIST_STEPS_KEY, 0);
    get_internal_data()->steps = 0;
    persist_write_bool(PERSIST_GOAL_MET_KEY, false);
    update_steps_display();
    vibes_short_pulse();
  }
  window_stack_pop(true);
}

static void reset_steps_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_reset_steps_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_reset_steps_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = reset_steps_menu_get_num_rows_callback,
    .draw_row = reset_steps_menu_draw_row_callback,
    .get_header_height = reset_steps_menu_get_header_height_callback,
    .draw_header = reset_steps_menu_draw_header_callback,
    .select_click = reset_steps_menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_reset_steps_menu, window);
  layer_add_child(window_layer, menu_layer_get_layer_jf(s_reset_steps_menu));
}

static void reset_steps_window_unload(Window *window) {
  menu_layer_destroy(s_reset_steps_menu);
  window_destroy(window);
  s_reset_steps_window = NULL;
}

static void show_reset_steps_confirmation() {
  s_reset_steps_window = window_create();
  window_set_window_handlers(s_reset_steps_window, (WindowHandlers) {
    .load = reset_steps_window_load,
    .unload = reset_steps_window_unload,
  });
  window_stack_push(s_reset_steps_window, true);
}

/*
 * This function is no longer needed as the clock is not displayed.
 * Display the clock on movement (ensures if you start moving the clock is up to date)
 * Also fired from button press
 */
EXTFN void revive_clock_on_movement(uint16_t last_movement) {
  if (last_movement >= CLOCK_UPDATE_THRESHOLD) {
    // Removed clock update logic as clock is not displayed
  }
}

/**
 * Back button single click handler
 */
static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  close_morpheuz();  
}

/*
 * Single click handler on down button
 */
static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_sleep_chart(); // Show sleep chart on DOWN press
}

/*
 * Single click handler on select button
 */
static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_reset_steps_confirmation(); // Chiede se resettare i passi
}

/*
 * Single click handler on up button
 */
static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_steps_chart(); // Show the steps chart when the UP button is pressed
}

/*
 * Button config
 */
static void click_config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}

/**
 * Indicate if title animation complete
 */
EXTFN bool is_animation_complete() {
  return ui.animation_count == 6;
}

/*
 * Stuff we only allow after we've gone through the normal pre-amble
 */
EXTFN void post_init_hook(void *data) {
  wakeup_init();
  update_steps_display();
  update_sleep_display();
 
  // Set click provider
  window_set_click_config_provider(ui.primary_window, (ClickConfigProvider) click_config_provider);
}

/*
 * Archivia i dati giornalieri (passi e sonno) nella cronologia
 * Questa funzione viene chiamata quando una sessione di sonno termina o viene resettata.
 */
EXTFN void archive_daily_data() {
    // Assicurati che ci sia un giorno precedente da archiviare
    if (previous_mday == -1) {
        return;
    }

    InternalData *id = get_internal_data();
    uint32_t light_mins = 0;
    uint32_t deep_mins = 0;
    
    // Calcola il sonno per il giorno che si è appena concluso
    for (int j = 0; j <= id->highest_entry; j++) {
      if (!id->ignore[j] && id->points[j] < LIGHT_ABOVE) {
        if (id->points[j] <= DEEP_SLEEP_THRESHOLD) deep_mins += 10;
        else light_mins += 10;
      }
    }

    // Archivia la cronologia del sonno (sposta i dati vecchi, salva i nuovi)
    for (int k = NUM_DAILY_STEP_HISTORY_DAYS - 1; k > 0; k--) {
      persist_write_int(PERSIST_DAILY_LIGHT_0 + k, persist_read_int(PERSIST_DAILY_LIGHT_0 + k - 1));
      persist_write_int(PERSIST_DAILY_DEEP_0 + k, persist_read_int(PERSIST_DAILY_DEEP_0 + k - 1));
    }
    persist_write_int(PERSIST_DAILY_LIGHT_0, light_mins);
    persist_write_int(PERSIST_DAILY_DEEP_0, deep_mins);

    // Aggiorna la data base per la cronologia del sonno
    // Questo timestamp serve come riferimento per le date nel grafico
    time_t yesterday = time(NULL) - (24 * 60 * 60);
    persist_write_int(PERSIST_DAILY_STEPS_BASE_KEY, yesterday);

}

  /*
 * Set the icon state for any icon
 */
EXTFN void set_icon(bool enabled, IconState icon) {
  if (enabled != icon_state[icon]) {
    icon_state[icon] = enabled;
    layer_mark_dirty(ui.icon_bar);
  }
}

/*
 * Get the icon state
 */
EXTFN bool get_icon(IconState icon) {
  return icon_state[icon];
}

/*
 * Are we monitoring sleep (recording or powernap)? 
 * Also now includes case where we have stopped recording and are ringing the alarm (including snoozed alarm)
 */
EXTFN bool is_monitoring_sleep() {
  return get_icon(IS_RECORD);
}

/*
 * Build an icon
 */
static void paint_icon(GContext *ctx, int *running_horizontal, int width, uint32_t resource_id, BmpCache cacheId) {
  GBitmap *bitmap = gbitmap_create_with_resource_cache(resource_id, cacheId);
  *running_horizontal -= width + ICON_PAD;
  graphics_draw_bitmap_in_rect(ctx, bitmap, GRect(*running_horizontal, 0, width, 12));
  gbitmap_destroy_cache(bitmap);
}

/*
 * Battery icon callback handler (called via icon bar update now)
 */
static void battery_layer_update_callback(Layer *layer, GContext *ctx, int *running_horizontal) {

  graphics_context_set_compositing_mode(ctx, GCompOpAssign);

  if (!ui.battery_plugged) {
    paint_icon(ctx, running_horizontal, 24, RESOURCE_ID_BATTERY_ICON, BMP_CACHE_BATTERY_ICON);
    graphics_context_set_stroke_color(ctx, BACKGROUND_COLOR);
    graphics_context_set_fill_color(ctx, BATTERY_BAR_COLOR);
    graphics_fill_rect(ctx, GRect(*running_horizontal + 7, 4, ui.battery_level / 9, 4), 0, GCornerNone);
  } else {
    paint_icon(ctx, running_horizontal, 24, RESOURCE_ID_BATTERY_CHARGE, BMP_CACHE_BATTERY_CHARGE);
  }
}

/*
 * Icon bar update handler
 */
EXTFN void icon_bar_update_callback(Layer *layer, GContext *ctx) {

  int running_horizontal = ICON_BAR_WIDTH;

  graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  // Don't draw if we're currently doing
  if (!is_animation_complete())
    return;

  // Battery icon (always showing)
  battery_layer_update_callback(layer, ctx, &running_horizontal);
  running_horizontal += ICON_PAD_BATTERY;

  // Comms icon / Bluetooth icon
  if (icon_state[IS_COMMS] || icon_state[IS_BLUETOOTH]) {
    paint_icon(ctx, &running_horizontal, 9, icon_state[IS_COMMS] ? RESOURCE_ID_COMMS_ICON : RESOURCE_ID_BLUETOOTH_ICON,
               icon_state[IS_COMMS] ? BMP_CACHE_COMMS_ICON : BMP_CACHE_BLUETOOTH_ICON);
  }
  
  // Record icon
  if (icon_state[IS_RECORD]) {
    paint_icon(ctx, &running_horizontal, 10, RESOURCE_ID_ICON_RECORD, BMP_CACHE_ICON_RECORD);
  }

  // Ignore icon
  if (icon_state[IS_IGNORE]) {
    paint_icon(ctx, &running_horizontal, 9, RESOURCE_ID_IGNORE, BMP_CACHE_IGNORE );
  }

}

/*
 * Battery state change
 */
static void battery_state_handler(BatteryChargeState charge) {
  ui.battery_level = charge.charge_percent;
  ui.battery_plugged = charge.is_plugged;
  layer_mark_dirty(ui.icon_bar);
}

/*
 * Progress line
 */
EXTFN void progress_layer_update_callback(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, BACKGROUND_COLOR);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, BAR_CHART_MARKS);

  graphics_context_set_stroke_color(ctx, BAR_CHART_MARKS);
  
  graphics_context_set_stroke_width(ctx, 2);

  uint8_t i;
  for (i = 0; i <= 120; i += 12) {
    graphics_draw_pixel(ctx, GPoint(i, 8));
    graphics_draw_pixel(ctx, GPoint(i, 7));
  }

  for (i = 0; i <= get_internal_data()->highest_entry; i++) {
    if (!get_internal_data()->ignore[i]) {
      uint16_t height = get_internal_data()->points[i] / 500;
      uint8_t i2 = i * 2;
      graphics_draw_line(ctx, GPoint(i2, 8 - height), GPoint(i2, 8));
    }
  }
}



/*
 * Process clockface
 */
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

  // Aggiornamento grafico della data a mezzanotte
  static int last_ui_mday = -1;
  if (tick_time->tm_mday != last_ui_mday) {
    strftime(date_text, sizeof(date_text), DATE_FORMAT, tick_time);
    last_ui_mday = tick_time->tm_mday;
  }

  // Esegui il reset solo se il giorno è cambiato E il Bluetooth è attualmente attivo
  if (tick_time->tm_mday != previous_mday && bluetooth_connection_service_peek()) {
    if (previous_mday != -1) {
      perform_daily_reset();
    }
    previous_mday = tick_time->tm_mday;
    persist_write_int(PERSIST_LAST_MDAY, previous_mday);
  }

  #ifndef TESTING_MEMORY_LEAK
  #else
    // Use the line to show heap space in testing mode
    snprintf(date_text, sizeof(date_text), "%d", heap_bytes_free());
    text_layer_set_text(ui.text_date_smart_alarm_range_layer, date_text);
  #endif

  // every_minute_processing() è già chiamato qui e gestisce il monitoraggio del sonno
  every_minute_processing(); // Gestione dati sonno e trasmissione punti (1 volta al minuto)
  update_steps_display();
  update_sleep_display();
}

/*
 * Bluetooth connection status
 */
static void bluetooth_state_handler(bool connected) {
  set_icon(connected, IS_BLUETOOTH);
  if (connected) {
    // Al momento della connessione, controlla se è necessario un reset giornaliero pendente
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t->tm_mday != previous_mday) {
      if (previous_mday != -1) {
        perform_daily_reset();
      }
      previous_mday = t->tm_mday;
      persist_write_int(PERSIST_LAST_MDAY, previous_mday);
    }
  } else {
    set_icon(false, IS_COMMS); // because we only set comms state on NAK/ACK it can be at odds with BT state - do this else that is confusing
  }
  update_sleep_display();
}

/*
 * Progress indicator position (1-54)
 */
EXTFN void set_progress() {
  if (!get_config_data()->analogue)
    layer_mark_dirty(ui.progress_layer);
}

/*
 * Common stuff we always do at the end of setup
 */
EXTFN void morpheuz_load_standard_postamble() {
  read_internal_data();
  read_config_data();
  
  // Carica l'ultimo giorno gestito per rilevare cambi di data (es. app chiusa a mezzanotte)
  if (persist_exists(PERSIST_LAST_MDAY)) {
    previous_mday = persist_read_int(PERSIST_LAST_MDAY);
  }

  // Inizializza la base della cronologia se non esiste (fondamentale per i primi grafici)
  if (!persist_exists(PERSIST_DAILY_STEPS_BASE_KEY)) {
    persist_write_int(PERSIST_DAILY_STEPS_BASE_KEY, time(NULL));
  }

  // Start clock
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  battery_state_handler(battery_state_service_peek());
  battery_state_service_subscribe(&battery_state_handler);

  bluetooth_state_handler(bluetooth_connection_service_peek());
  bluetooth_connection_service_subscribe(bluetooth_state_handler);

  init_morpheuz();

  // Aggiorna immediatamente lo schermo con i dati caricati
  update_steps_display();
  update_sleep_display();

  // Avvia il timer per l'invio dati a Home Assistant ogni 5 secondi
  ha_timer = app_timer_register(5000, ha_timer_handler, NULL);

  light_enable_interaction();
}