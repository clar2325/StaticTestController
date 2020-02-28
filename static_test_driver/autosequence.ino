// Timing
#define PRESTAGE_PREP_TIME 0 // Time at which to open the prestage valves
#define PRESTAGE_TIME      0
#define MAINSTAGE_TIME     0
#define THRUST_CHECK_TIME  2000 // Time at which to start checking the engine is producing thrust, etc.
#define OX_LEADTIME        500  // Delay between closing oxygen and closing fuel prestage
#define PRE_LEADTIME       1000 // Delay between closing oxygen prestage and closing both mainstage
#define HEARTBEAT_TIMEOUT  1000 // Timeout to abort the run after not recieving a heartbeat signal

#if CONFIGURATION == DEMO
#define COUNTDOWN_DURATION 10000 // 10 sec
#define RUN_TIME           10000 // 10 sec
#define COOLDOWN_TIME      10000 // 10 sec
#else
#define COUNTDOWN_DURATION 60000 // 1 min
#define RUN_TIME           12000 // 12 sec
#define COOLDOWN_TIME      60000 * 5 // 5 mins
#endif

// Limits
// Max outlet temperature before triggering a shutdown
#define MAX_COOLANT_TEMP 60

// Min thrust that must be reached to avoid triggering a no-ignition shutdown
#if CONFIGURATION == DEMO
#define MIN_THRUST 50
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
  OXYGEN_SHUTDOWN,
  SHUTDOWN,
  COOL_DOWN
} state_t;

// Convenience
#define SET_STATE(STATE) {    \
    state = STATE;            \
    write_state(#STATE);      \
  }
  

long start_time = 0;
long shutdown_time = 0;
long heartbeat_time = 0;
state_t state = STAND_BY;

void write_state(const char *state_name) {
  set_lcd_status(state_name);
  SEND(status, state_name);
}

void blink(int led, long period) {
  digitalWrite(led, (int)((millis() % (period * 2)) / period));
}

void heartbeat() {
  heartbeat_time = millis();
}

void init_autosequence() {
  SET_STATE(STAND_BY);
}

void start_countdown() {
  #if CONFIGURATION != DEMO
  if (!sensor_status) {
    Serial.println(F("Countdown aborted due to sensor failure"));
    SET_STATE(STAND_BY) // Set state to signal countdown was aborted
  } else
  #endif
  if (valve_status[FUEL_PRE] ||
      valve_status[FUEL_MAIN] ||
      valve_status[OX_PRE] ||
      valve_status[OX_MAIN]) {
    Serial.println(F("Countdown aborted due to unexpected initial state"));
    SET_STATE(STAND_BY) // Set state to signal countdown was aborted
  }
  else {
    Serial.println(F("Countdown started"));
    SET_STATE(TERMINAL_COUNT)
    start_time = millis();
    heartbeat();
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
      set_valve(OX_PRE, 0);
      SET_STATE(STAND_BY)
      break;

    case PRESTAGE:
      set_valve(OX_PRE, 0);
      set_valve(FUEL_PRE, 0);
      reset_igniter();
      SET_STATE(COOL_DOWN)
      shutdown_time = millis();
      break;
    
    case MAINSTAGE:
      set_valve(OX_PRE, 0);
      set_valve(FUEL_PRE, 0);
      SET_STATE(SHUTDOWN)
      shutdown_time = millis();
      break;
  }
}

void run_control() {
  long run_time = millis() - start_time - COUNTDOWN_DURATION;
  SEND(run_time, run_time)

  handle_igniter();

#if CONFIGURATION == MK_2
  if (state != STAND_BY && state != COOL_DOWN && millis() > heartbeat_time + HEARTBEAT_TIMEOUT) {
    Serial.println(F("Loss of data link"));
    abort_autosequence();
  }
#endif

  switch (state) {
    case STAND_BY:
      // State that waits for a person to begin the test
      if (!sensor_status) {
        set_lcd_status("Sensor failure");
      }
      break;
    case TERMINAL_COUNT:
      // Countdown state
      #if CONFIGURATION != DEMO
      if (!sensor_status) {
        Serial.println(F("Sensor failure"));
        abort_autosequence();
      }
      else
      #endif
      if (run_time >= PRESTAGE_PREP_TIME) {
        SET_STATE(PRESTAGE_READY)
        set_valve(FUEL_PRE, 1);
        set_valve(OX_PRE, 1);
      }
      break;
    case PRESTAGE_READY:
      // State to wait for ignition after prestage valves are open
      if (run_time >= PRESTAGE_TIME) {
        SET_STATE(PRESTAGE)
        fire_igniter();
      }
      break;
    case PRESTAGE:
      // State to wait for mainstage valves to be opened after ignition
      if (run_time >= MAINSTAGE_TIME) {
        SET_STATE(MAINSTAGE)
        set_valve(FUEL_MAIN, 1);
        set_valve(OX_MAIN, 1);
      }
      break;
    case MAINSTAGE:
      // State for active firing of the engine
      #if CONFIGURATION != DEMO
      // Check that the sensors are still working
      if (!sensor_status) {
        Serial.println(F("Sensor failure"));
        abort_autosequence();
      }
      else
      #endif
      // Check that the coolant temperature has not exceeded the maximum limit
      if (outlet_temp >= MAX_COOLANT_TEMP) {
        Serial.println(F("Temperature reached critical level"));
        abort_autosequence();
      }
      // Check that the engine is producing thrust once mainstage valves are fully open
      else if (run_time >= THRUST_CHECK_TIME && force < MIN_THRUST) {
        Serial.println(F("Thrust below critical level"));
        abort_autosequence();
      }

      if (run_time >= RUN_TIME) {
        SET_STATE(OXYGEN_SHUTDOWN)
        set_valve(OX_PRE, 0);
        shutdown_time = millis();
      }
      break;

    case OXYGEN_SHUTDOWN:
      // Oxygen prestage valve is closed, others are still open
      if (millis() >= shutdown_time + OX_LEADTIME) {
        set_valve(FUEL_PRE, 0);
        SET_STATE(SHUTDOWN)
      }
      break;

    case SHUTDOWN:
      // Both prestage valves are closed, others may still be open
      if (millis() >= shutdown_time + PRE_LEADTIME) {
        set_valve(OX_MAIN, 0);
        set_valve(FUEL_MAIN, 0);
        SET_STATE(COOL_DOWN)
      }
      break;

    case COOL_DOWN:
      // State that waits for cool-down to take place after a run is complete
      if (millis() - shutdown_time >= COOLDOWN_TIME) {
        Serial.println(F("Run finished"));
        SET_STATE(STAND_BY)
        start_time = 0;
      }
      break;
  }
}
