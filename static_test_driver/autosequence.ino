
#define COUNTDOWN_DURATION 60000
#define SAFETY_TIME -10000
#define START_TIME  0
#define RUN_TIME  30000
#define COOLDOWN_TIME 60000 * 5

// TODO: I don't know if we need more states?  
enum state {
  MANUAL_CONTROL,
  TERMINAL_COUNT,
  ENGINE_READY,
  ENGINE_RUNNING,
  COOL_DOWN
};

// Convinence
#define SET_STATE(STATE) {\
  state = STATE;          \
  SEND(status, #STATE);   \
}

long start_time = 0;
long shutdown_time = 0;
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
      fuel_safety(false);
      oxy_safety(false);
      SET_STATE(MANUAL_CONTROL)
      break;
    case ENGINE_RUNNING:
      SET_STATE(COOL_DOWN)
      fuel_throttle(0);
      oxy_throttle(0);
      fuel_safety(false);
      oxy_safety(false);
      shutdown_time = millis();
      break;
    // etc, TODO
  }
}

void run_control() {
  if (state == MANUAL_CONTROL)
    run_manual_control();
  else {
    long run_time = millis() - start_time - COUNTDOWN_DURATION;
    SEND(run_time, run_time)

    // TODO: There should be a lot more here, also checking if things went wrong
    switch (state) {
      case TERMINAL_COUNT:
        if (run_time >= SAFETY_TIME) {
          SET_STATE(ENGINE_READY)
          fuel_safety(true);
          oxy_safety(true);
        }
        break;
        
      case ENGINE_READY:
        if (run_time >= START_TIME) {
          SET_STATE(ENGINE_RUNNING)
          fuel_throttle(10);
          oxy_throttle(10);
          fire_ignitor();
        }
        break;
      
      case ENGINE_RUNNING:
        if (run_time >= RUN_TIME) {
          SET_STATE(COOL_DOWN)
          fuel_throttle(0);
          oxy_throttle(0);
          fuel_safety(false);
          oxy_safety(false);
          shutdown_time = millis();
        }
        break;
      
      case COOL_DOWN:
        if (millis() - shutdown_time >= COOLDOWN_TIME) {
          Serial.println("Autosequencer finished");
          SET_STATE(MANUAL_CONTROL)
          start_time = 0;
        }
        break;
    }
  }
}

void run_manual_control() {
  // TODO: Read and update throttle settings
}

