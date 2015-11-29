/* -*-coding: utf-8 -*-
 * vim: sw=2 ts=2 expandtab ai
 *
 * *********************************
 * * Pebble Talk2Web App           *
 * * by bahbka <bahbka@bahbka.com> *
 * *********************************/

#include <pebble.h>

#define KEY_START_IMMEDIATELY 10
#define KEY_ENABLE_CONFIRMATION_DIALOG 20
#define KEY_ENABLE_ERROR_DIALOG 30
#define KEY_STATUS 100
#define KEY_TEXT 110

#define REQUEST_TIMEOUT 30*1000
#define EXIT_TIMEOUT 3*1000

static Window *s_main_window;
static TextLayer *s_output_layer;

static DictationSession *s_dictation_session;
static char s_last_text[1024];

static AppTimer *exit_timer = NULL;
static AppTimer *timeout_timer = NULL;

static void show_text(char *message, GColor fg_color, GColor bg_color) {
  text_layer_set_text_color(s_output_layer, fg_color);
  text_layer_set_background_color(s_output_layer, bg_color);
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(s_output_layer, message);
}

static void exit_timeout() {
  window_stack_pop_all(true);
}

static void request_timeout() {
  show_text("request timeout", GColorFromHEX(0xffffff), GColorFromHEX(0xff0000));
  timeout_timer = app_timer_register(EXIT_TIMEOUT, exit_timeout, NULL);
}

static void send_text(char *transcription) {
  snprintf(s_last_text, sizeof(s_last_text), "sending\n%s", transcription);
  show_text(s_last_text, GColorFromHEX(0x44ff44), GColorFromHEX(0x000000));

  DictionaryIterator *iterator;
  app_message_outbox_begin(&iterator);
  dict_write_cstring(iterator, KEY_TEXT, transcription);
  app_message_outbox_send();

  timeout_timer = app_timer_register(REQUEST_TIMEOUT, request_timeout, NULL);
}

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context) {
  if(status == DictationSessionStatusSuccess) {
    // send recognized text
    send_text(transcription);

  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "dictation error %d", (int)status);
    // display the reason for any error
    static char s_failed_buff[128];
    snprintf(s_failed_buff, sizeof(s_failed_buff), "failed\nerror %d", (int)status);
    switch((int)status) {
      case 1:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nTranscription\nRejected\nerrno %d", (int)status);
        break;
      case 2:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nTranscription\nRejected\nWith Error\nerrno %d", (int)status);
        break;
      case 3:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nSystem\nAborted\nerrno %d", (int)status);
        break;
      case 4:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nNo Speech\nDetected\nerrno %d", (int)status);
        break;
      case 5:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nConnectivity\nError\nerrno %d", (int)status);
        break;
      case 6:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nDisabled\nerrno %d", (int)status);
        break;
      case 7:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nInternal\nError\nerrno %d", (int)status);
        break;
      case 8:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nRecognizer\nError\nerrno %d", (int)status);
        break;
      default:
        snprintf(s_failed_buff, sizeof(s_failed_buff), "Failure\nUnknown\nerrno %d", (int)status);
    }
    show_text(s_failed_buff, GColorFromHEX(0xffffff), GColorFromHEX(0xff0000));
    timeout_timer = app_timer_register(EXIT_TIMEOUT, exit_timeout, NULL);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "message received");

  Tuple *start_immediately = dict_find(iterator, KEY_START_IMMEDIATELY);
  if (start_immediately) {
    APP_LOG(APP_LOG_LEVEL_INFO, "remember KEY_START_IMMEDIATELY=%d", start_immediately->value->uint8);
    persist_write_bool(KEY_START_IMMEDIATELY, start_immediately->value->uint8);
  }

  Tuple *enable_confirmation_dialog = dict_find(iterator, KEY_ENABLE_CONFIRMATION_DIALOG);
  if (enable_confirmation_dialog) {
    APP_LOG(APP_LOG_LEVEL_INFO, "remember KEY_ENABLE_CONFIRMATION_DIALOG=%d", enable_confirmation_dialog->value->uint8);
    persist_write_bool(KEY_ENABLE_CONFIRMATION_DIALOG, enable_confirmation_dialog->value->uint8);
    dictation_session_enable_confirmation(s_dictation_session, persist_read_bool(KEY_ENABLE_CONFIRMATION_DIALOG));
  }

  Tuple *enable_error_dialog = dict_find(iterator, KEY_ENABLE_ERROR_DIALOG);
  if (enable_error_dialog) {
    APP_LOG(APP_LOG_LEVEL_INFO, "remember KEY_ENABLE_ERROR_DIALOG=%d", enable_error_dialog->value->uint8);
    persist_write_bool(KEY_ENABLE_ERROR_DIALOG, enable_error_dialog->value->uint8);
    dictation_session_enable_error_dialogs(s_dictation_session, persist_read_bool(KEY_ENABLE_ERROR_DIALOG));
  }

  Tuple *status = dict_find(iterator, KEY_STATUS);
  Tuple *text = dict_find(iterator, KEY_TEXT);
  if (status) {
    if (status->value->uint8 == 0) {
      if (text) {
        show_text(text->value->cstring, GColorFromHEX(0x000000), GColorFromHEX(0x00ff00));
      } else {
        show_text("OK", GColorFromHEX(0x000000), GColorFromHEX(0x00ff00));
      }
    } else {
      if (text) {
        show_text(text->value->cstring, GColorFromHEX(0xffffff), GColorFromHEX(0xff0000));
      } else {
        show_text("ERROR", GColorFromHEX(0xffffff), GColorFromHEX(0xff0000));
      }
    }
    timeout_timer = app_timer_register(EXIT_TIMEOUT, exit_timeout, NULL);
  }
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
  if (timeout_timer)
    app_timer_cancel(timeout_timer);
  if (exit_timer)
    app_timer_cancel(exit_timer);
  dictation_session_start(s_dictation_session);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // TODO scroll layer?
  //s_output_layer = text_layer_create(GRect(bounds.origin.x, (bounds.size.h - 24) / 2, bounds.size.w, bounds.size.h));
  s_output_layer = text_layer_create(GRect(bounds.origin.x + 5, bounds.origin.y + 5, bounds.size.w - 10, bounds.size.h - 10));
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
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

  if (persist_exists(KEY_ENABLE_CONFIRMATION_DIALOG)) {
    dictation_session_enable_confirmation(s_dictation_session, persist_read_bool(KEY_ENABLE_CONFIRMATION_DIALOG));
  }

  if (persist_exists(KEY_ENABLE_ERROR_DIALOG)) {
    dictation_session_enable_error_dialogs(s_dictation_session, persist_read_bool(KEY_ENABLE_ERROR_DIALOG));
  }

  if (persist_exists(KEY_START_IMMEDIATELY) && persist_read_bool(KEY_START_IMMEDIATELY)) {
    dictation_session_start(s_dictation_session);
  } else {
    show_text("Press SELECT button to start", GColorFromHEX(0xffffff), GColorFromHEX(0x000000));
  }
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
