// Timing
#define PRESTAGE_PREP_TIME -1000 // Time at which to open the prestage valves
#define PRESTAGE_TIME      0
#define MAINSTAGE_TIME     1000
#define STARTUP_TIME       3000  // Time at which to start checking the engine is producing thrust, etc.

#if CONFIGURATION == DEMO
#define COUNTDOWN_DURATION 10000 // 10 sec
#define RUN_TIME           10000 // 10 sec
#define COOLDOWN_TIME      10000 // 10 sec
#else
#define COUNTDOWN_DURATION 60000 // 1 min
#define RUN_TIME           30000 // 30 sec
#define COOLDOWN_TIME      60000 * 5 // 5 mins
#endif

// Limits
// Max outlet temperature before triggering a shutdown
#define MAX_COOLANT_TEMP 60

// Min thrust that must be reached to avoid triggering a no-ignition shutdown
#if CONFIGURATION == DEMO
#define MIN_THRUST 100
#elif CONFIGURATION == MK_1
#define MIN_THRUST 100
#elif CONFIGURATION == MK_2
#define MIN_THRUST 1000
#endif

// State LED
#define TERMINAL_COUNT_LED_PERIOD 250
#define COOL_DOWN_LED_PERIOD 1000

typedef enum {
  STAND_BY,
  TERMINAL_COUNT,
  PRESTAGE_READY,
  PRESTAGE,
  MAINSTAGE,
  COOL_DOWN
} state_t;

// Convenience
#define SET_STATE(STATE) {\
    state = STATE;          \
    SEND(status, #STATE);   \
  }

long start_time = 0;
long shutdown_time = 0;
state_t state = STAND_BY;

void start_countdown() {
  if (!sensor_status) {
    Serial.println(F("Countdown aborted due to sensor failure"));
    SET_STATE(STAND_BY) // Set state to signal countdown was aborted
  }
  else if (valve_status[FUEL_PRE] ||
           valve_status[FUEL_MAIN] ||
           valve_status[OXY_PRE] ||
           valve_status[OXY_MAIN]) {
    Serial.println(F("Countdown aborted due to unexpected initial state"));
    SET_STATE(STAND_BY) // Set state to signal countdown was aborted
  }
  else {
    Serial.println(F("Countdown started"));
    SET_STATE(TERMINAL_COUNT)
    start_time = millis();
  }
}

void abort_autosequence() {
  Serial.println(F("Run aborted"));
  switch (state) {
    case STAND_BY:
    case TERMINAL_COUNT:
      SET_STATE(STAND_BY)
      break;

    case PRESTAGE_READY:
      set_valve(FUEL_PRE, 0);
      set_valve(OXY_PRE, 0);
      break;

    case PRESTAGE:
    case MAINSTAGE:
      SET_STATE(COOL_DOWN)
      set_valve(FUEL_PRE, 0);
      set_valve(FUEL_MAIN, 0);
      set_valve(OXY_PRE, 0);
      set_valve(OXY_MAIN, 0);
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
      // State that waits for a person to begin the test
      break;
    case TERMINAL_COUNT:
      // Countdown state
      if (run_time >= PRESTAGE_PREP_TIME) {
        SET_STATE(PRESTAGE_READY)
        set_valve(FUEL_PRE, 1);
        set_valve(OXY_PRE, 1);
      }
      break;
    case PRESTAGE_READY:
      // State to wait for ignition after prestage valves are open
      if (run_time >= MAINSTAGE_TIME) {
        SET_STATE(PRESTAGE)
        fire_ignitor();
      }
      break;
    case PRESTAGE:
      // State to wait for mainstage valves to be opened after ignition
      if (run_time >= MAINSTAGE_TIME) {
        SET_STATE(MAINSTAGE)
        set_valve(FUEL_MAIN, 1);
        set_valve(OXY_MAIN, 1);
      }
      break;
    case MAINSTAGE:
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
      // Check that the engine is producing thrust once mainstage valves are fully open
#if CONFIGURATION != DEMO
      else if (run_time >= STARTUP_TIME && force < MIN_THRUST) {
#else
      else if (force < MIN_THRUST) {
#endif
        Serial.println(F("Thrust below critical level"));
        abort_autosequence();
      }

      if (run_time >= RUN_TIME) {
        SET_STATE(COOL_DOWN)
        set_valve(FUEL_PRE, 0);
        set_valve(FUEL_MAIN, 0);
        set_valve(OXY_PRE, 0);
        set_valve(OXY_MAIN, 0);
        shutdown_time = millis();
      }
      break;

    case COOL_DOWN:
      // State that waits for cool-down to take place after a run is complete
      digitalWrite(STATE_LED, (millis() % COOL_DOWN_LED_PERIOD) * 2 / COOL_DOWN_LED_PERIOD);
      if (millis() - shutdown_time >= COOLDOWN_TIME) {
        Serial.println(F("Run finished"));
        SET_STATE(STAND_BY)
        start_time = 0;
      }
      break;
  }
}

