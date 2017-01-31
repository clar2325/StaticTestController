// Timing
#define COUNTDOWN_DURATION 60000 // 1 min
#define START_TIME  0
#define RUN_TIME  30000
#define COOLDOWN_TIME 60000 * 5 // 5 mins

#define TEMP_CHECK_TIME 5000

// Limits
#define MAX_COOLANT_TEMP 60
#define MIN_COOLANT_DELTA 8 // Min delta temp between inlet and outlet after TEMP_CHECK_TIME

enum state {
  STAND_BY,
  TERMINAL_COUNT,
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
enum state state = STAND_BY;

//changed start_autosequence to start_countdown which is called in the standby state
void start_countdown() {
  Serial.println(F("Countdown started"));
  SET_STATE(TERMINAL_COUNT)
  start_time = millis();
}

void abort_autosequence() {
  Serial.println(F("Autosequencer aborted"));
  switch (state) {
    case STAND_BY:
    case TERMINAL_COUNT:
      SET_STATE(STAND_BY)
      break;

    case ENGINE_RUNNING:
      SET_STATE(COOL_DOWN)
      fuel_throttle(0);
      oxy_throttle(0);
      shutdown_time = millis();
      break;
  }
}

void run_control() {
  long run_time = millis() - start_time - COUNTDOWN_DURATION;
  SEND(run_time, run_time)
  
  // TODO: There should be a lot more here, also checking if things went wrong
  switch (state) {
    case STAND_BY:
      //state that waits for a person to begin the test
      break;
    case TERMINAL_COUNT:
      //countdown state
      if (run_time >= START_TIME) {
        SET_STATE(ENGINE_RUNNING)
        fuel_throttle(10);
        oxy_throttle(10);
        fire_ignitor();
      }
      break;
    case ENGINE_RUNNING:

      // Check that the sensors are still working
      if (!sensor_status) {
        Serial.println(F("Sensor failure"));
        abort_autosequence();
      }
      // Check that the coolant temperature has not exceeded the maximum limit
      else if (outlet_temp >= MAX_COOLANT_TEMP) {
        Serial.println(F("Temperature Reached Critical level"));
        abort_autosequence();
      }
      // looked at past tests and put in some rough parameters for ignition. The temperature would rise by approx 8 degrees celsius in the span of 2-3 seconds
      // checking outlet temperature to see if it reaches a certain level
      else if (run_time >= TEMP_CHECK_TIME && outlet_temp - inlet_temp < MIN_COOLANT_DELTA) {
        Serial.println(F("Temperature Reached Critical level"));
        abort_autosequence();
      }
      
      if (run_time >= RUN_TIME) {
        SET_STATE(COOL_DOWN)
        fuel_throttle(0);
        oxy_throttle(0);
        shutdown_time = millis();
      }
      break;
    
    case COOL_DOWN:
      if (millis() - shutdown_time >= COOLDOWN_TIME) {
        Serial.println(F("Autosequencer finished"));
        SET_STATE(STAND_BY)
        start_time = 0;
      }
      break;
  }
  update_throttle();
}

