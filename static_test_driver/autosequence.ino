// Timing
#define COUNTDOWN_DURATION 60000 // 1 min
#define START_TIME  0
#define RUN_TIME  30000
#define COOLDOWN_TIME 60000 * 5 // 5 mins

// Limits
// Max outlet temperature before triggering a shutdown
#define MAX_COOLANT_TEMP 60

// Min thrust that must be reached to avoid triggering a no-ignition shutdown
#if CONFIGURATION == DEMO || CONFIGURATION == MK_1
#define MIN_THRUST 100
#elif CONFIGURATION == MK_2
#define MIN_THRUST 1000
#endif

// State LED
#define TERMINAL_COUNT_LED_PERIOD 250
#define COOL_DOWN_LED_PERIOD 1000

enum state {
  STAND_BY,
  TERMINAL_COUNT,
  ENGINE_RUNNING,
  COOL_DOWN
};

// Convenience
#define SET_STATE(STATE) {\
    state = STATE;          \
    SEND(status, #STATE);   \
  }

long start_time = 0;
long shutdown_time = 0;
enum state state = STAND_BY;

void start_countdown() {
  if (sensor_status) {
    Serial.println(F("Countdown started"));
    SET_STATE(TERMINAL_COUNT)
    start_time = millis();
  }
  else {
    Serial.println(F("Countdown aborted due to sensor failure"));
    SET_STATE(STAND_BY) // Set state to signal countdown was aborted
  }
}

void abort_autosequence() {
  Serial.println(F("Run aborted"));
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
      // state that waits for a person to begin the test
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
        Serial.println(F("Temperature reached critical level"));
        abort_autosequence();
      }
      // Check that the engine is producing thrust once throttle valves are fully open
      else if (fuel_setting == fuel_target && oxy_setting == oxy_target && force < MIN_THRUST) {
        Serial.println(F("Thrust below critical level"));
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
      digitalWrite(STATE_LED, (millis() % COOL_DOWN_LED_PERIOD) * 2 / COOL_DOWN_LED_PERIOD);
      if (millis() - shutdown_time >= COOLDOWN_TIME) {
        Serial.println(F("Run finished"));
        SET_STATE(STAND_BY)
        start_time = 0;
      }
      break;
  }
  update_throttle();
}

