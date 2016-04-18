
#define COUNTDOWN_DURATION 60000
#define SAFETY_TIME -10000
#define START_TIME  0
#define RUN_TIME  30000
#define COOLDOWN_TIME 60000 * 5

enum state {
  MANUAL_CONTROL,
  TERMINAL_COUNT,
  ENGINE_READY,
  ENGINE_RUNNING,
  COOL_DOWN
};

char *convert_name(char *n) {
  for (int i = 1; i < strlen(n); i++) {
    if (n[i] == '_')
      n[i] = ' ';
    else
      n[i] = tolower(n[i]);
  }
  return n;
}

#define SET_STATE(STATE) {           \
  state = STATE;                     \
  SEND(status, convert_name(#STATE));\
}

long start_time = 0;
enum state state = MANUAL_CONTROL;


void start_autosequence() {
  Serial.println("Autosequencer started");
  SET_STATE(TERMINAL_COUNT)
  start_time = millis();
}

void abort_autosequence() {
  Serial.println("Autosequencer aborted");
  switch (state) {
    case TERMINAL_COUNT:
      SET_STATE(MANUAL_CONTROL)
      break;
    case ENGINE_READY:
      close_safety();
      SET_STATE(MANUAL_CONTROL)
      break;
    case ENGINE_RUNNING:
      SET_STATE(COOL_DOWN)
      throttle(0);
      close_safety();
      break;
    // etc
  }
}

void run_control() {
  if (state == MANUAL_CONTROL)
    run_manual_control();
  else {
    long run_time = millis() - start_time - COUNTDOWN_DURATION;
    SEND(run_time, run_time)
  
    switch (state) {
      case TERMINAL_COUNT:
        if (run_time >= SAFETY_TIME) {
          SET_STATE(ENGINE_READY)
          open_safety();
        }
        break;
        
      case ENGINE_READY:
        if (run_time >= START_TIME) {
          SET_STATE(ENGINE_RUNNING)
          throttle(10);
          fire_ignitor();
        }
        break;
      
      case ENGINE_RUNNING:
        if (run_time >= RUN_TIME) {
          SET_STATE(COOL_DOWN)
          throttle(0);
        }
        break;
      
      case COOL_DOWN:
        if (run_time >= COOLDOWN_TIME) {
          Serial.println("Autosequencer finished");
          SET_STATE(MANUAL_CONTROL)
          start_time = 0;
        }
        break;
    }
  }
}

void run_manual_control() {
  // TODO
}

