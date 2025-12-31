#ifndef GLOBAL_H_
#define GLOBAL_H_

// global state aka preferences, saved in non-volatile memory
typedef struct {
  bool enable;
  uint16_t pattern;
  bool timerEnable;
  byte onTimeH, onTimeM;	// local time H:M
  byte offTimeH, offTimeM;
} g_state_t;

extern g_state_t g_state;

void update_prefs();

// net
void init_wifi();
void init_time();
void init_webserver();
void check_webserver();

#endif
