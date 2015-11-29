/* -*-coding: utf-8 -*-
 * vim: sw=2 ts=2 expandtab ai
 *
 * *********************************
 * * Pebble Talk2Web App           *
 * * by bahbka <bahbka@bahbka.com> *
 * *********************************/

#include <pebble.h>

#define KEY_STATUS 10
#define KEY_TEXT 20

static Window *s_main_window;
static TextLayer *s_output_layer;

static DictationSession *s_dictation_session;
static char s_last_text[1024];

static void notify(char *message, GColor color) {
  text_layer_set_text_color(s_output_layer, color);
  text_layer_set_background_color(s_output_layer, GColorFromHEX(0x000000));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(s_output_layer, message);
}

static void send_text(char *transcription) {
  notify("sending", GColorFromHEX(0x88ff88));
  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);
  dict_write_cstring(iterator, KEY_TEXT, transcription);
  app_message_outbox_send();
}

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
    // Display the dictated text
    snprintf(s_last_text, sizeof(s_last_text), "Transcription:\n\n%s", transcription);
    text_layer_set_text(s_output_layer, s_last_text);
    send_text(transcription);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "dictation error: %d", (int)status);
    // display the reason for any error
    static char s_failed_buff[128];
    snprintf(s_failed_buff, sizeof(s_failed_buff), "Transcription failed.\n\nError ID:\n%d", (int)status);
    text_layer_set_text(s_output_layer, s_failed_buff);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "message received");
  /*Tuple *text = dict_find(iterator, KEY_TEXT);
  Tuple *status = dict_find(iterator, KEY_STATUS);
  if (data) {
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA received with value %d", (int)data->value->int32);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "KEY_DATA not received.");
  }*/
  notify("", GColorFromHEX(0xff5555));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "message dropped");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "outbox send failed");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox send success");
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  //dictation_session_start(s_dictation_session);
  send_text("test");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //s_output_layer = text_layer_create(GRect(bounds.origin.x, (bounds.size.h - 24) / 2, bounds.size.w, bounds.size.h));
  s_output_layer = text_layer_create(GRect(bounds.origin.x + 5, bounds.origin.y + 5, bounds.size.w - 10, bounds.size.h - 10));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));

  notify("test", GColorFromHEX(0xff5555));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_output_layer);
}

void init(void) {
  // register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // open AppMessage with sensible buffer sizes
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // main window
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_background_color(s_main_window, GColorFromHEX(0x000000));
  window_stack_push(s_main_window, true);

  // dictation
  s_dictation_session = dictation_session_create(sizeof(s_last_text), dictation_session_callback, NULL);
  dictation_session_enable_confirmation(s_dictation_session, false); // TODO configure this
  dictation_session_enable_error_dialogs(s_dictation_session, false); // TODO configure this

  //dictation_session_start(s_dictation_session); // TODO configure this
}

void deinit(void) {
  dictation_session_destroy(s_dictation_session);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
