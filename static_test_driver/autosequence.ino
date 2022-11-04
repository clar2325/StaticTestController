#define DEMO       1    // demo run
#define MK_2_FULL  2    // Test fire for Mk.2 full capacity (new ox regulator)
#define MK_2_LOW   3    // Test fire for Mk.2 low capacity  (old ox regulator)

#define CONFIGURATION MK_2_LOW

#define PRESTAGE_PREP_TIME 0              // Time at which to open the prestage valves
#define PRESTAGE_TIME      0
#define MAINSTAGE_TIME     0
#define THRUST_CHECK_TIME  2000           // Time at which to start measuring engine output thrust (2 seconds)
#define OX_LEADTIME        500            // Delay between closing oxygen prestage valve and closing fuel prestage valve (0.5 seconds)
#define PRE_LEADTIME       1000           // Delay between closing oxygen prestage valve and closing both mainstage valves (1 second)
#define HEARTBEAT_TIMEOUT  1000           // Timeout to abort run after not recieving heartbeat signal (1 second)

#if CONFIGURATION == DEMO
  #define COUNTDOWN_DURATION  10000       // 10 seconds
  #define RUN_TIME            10000       // 10 seconds
  #define COOLDOWN_TIME       10000       // 10 seconds
#elseif CONFIGURATION == MK_2_FULL
  #define COUNTDOWN_DURATION  60000       // 1 minute
  #define RUN_TIME            10000       // 10 seconds
  #define COOLDOWN_TIME       60000 * 10  // 10 minutes
#elseif CONFIGURATION == MK_2_LOW
  #define COUNTDOWN_DURATION  60000       // 1 minute
  #define RUN_TIME            2000        // 2 seconds
  #define COOLDOWN_TIME       60000 * 5   // 5 minutes
#endif

//***Limits***//
#define MAX_COOLANT_TEMP 60               // states max outlet temperature (60°C)

#if CONFIGURATION == DEMO
  #define MIN_THRUST  50
#elseif CONFIGURATION == MK_2_FULL
  #define MIN_THRUST  1000                // Min thrust that must be reached to avoid triggering a no-ignition shutdown (1000 Newtons)
#elseif CONFIGURATION == MK_2_LOW
  #define MIN_THRUST  100                 // Min thrust that must be reached to avoid triggering a no-ignition shutdown (100 Newtons)
#endif

//***LED's***//
#if CONFIGURATION == DEMO
  #define TERMINAL_COUNT_LED_PERIOD 10000       // 1 minute
  #define COOL_DOWN_LED_PERIOD      10000       // 1 minute
#elseif CONFIGURATION == MK_2_FULL
  #define TERMINAL_COUNT_LED_PERIOD 60000       // 1 minute
  #define COOL_DOWN_LED_PERIOD      60000 * 10  // 10 minutes
#elseif CONFIGURATION == MK_2_LOW
  #define TERMINAL_COUNT_LED_PERIOD 60000       // 1 minute
  #define COOL_DOWN_LED_PERIOD      60000 * 5   // 5 minutes
#endif


typedef enum{
  STAND_BY,
  TERMINAL_COUNT,
  PRESTAGE_READY,
  PRESTAGE,
  MAINSTAGE,
  OXYGEN_SHUTDOWN,
  SHUTDOWN,
  COOL_DOWN
}state_t;

#define SET_STATE(STATE){
  state = STATE;
  write_state(#STATE);
}

long start_time     = 0;
long shutdown_time  = 0;
long heartbeat_time = 0;

state_t state = STAND_BY;

void write_state(const char *state_name){
  set_lcd_status(state_name);
  SEND(status, state_name);
}

void blink(int led, long period){
  digitalWrite(led, (int)((millis() % (period * 2)) / period));
}

void heartbeat(){
  heartbeat_time = millis();
}

void init_autosequence(){
  SET_STATE(STAND_BY);
}

void start_countdown(){
  #if CONFIGURATION != DEMO
    if (sensor_status == false){
      Serial.println(F("Countdown aborted due to sensor failure"));
      SET_STATE(STAND_BY);
    }else
  #endif
  if (valve_fuel_pre.m_current_state  ||
      valve_fuel_main.m_current_state ||
      valve_ox_pre.m_current_state    ||
      valve_ox_main.m_current_state   ||
      valve_n2_choke.m_current_state  ||
      valve_n2_drain.m_current_state  ){
        Serial.println(F("Countdown aborted due to unexpected initial valve state."));
        SET_STATE(STAND_BY);
  }
  else{
    Serial.println(F("Countdown has started"));
    SET_STATE(TERMINAL_COUNT);
    start_time = millis();
    heartbeat();
  }
}

void abort_autosequence(){
  Serial.println(F("run aborted"));
  switch (state){
    case STAND_BY:
    case TERMINAL_COUNT:
      SET_STATE(STAND_BY);
      break;

    case PRESTAGE_READY:
      valve_n2_choke.set_valve(0);
      valve_fuel_pre.set_valve(0);
      valve_ox_pre.set_valve(0);
      SET_STATE(STAND_BY);
      break;

    case PRESTAGE:
      valve_n2_choke.set_valve(0);
      valve_fuel_pre.set_valve(0);
      valve_ox_pre.set_valve(0);
      reset_igniter();
      SET_STATE(COOL_DOWN);
      shutdown_time = millis();
      break;

    case MAINSTAGE:
      valve_n2_choke.set_valve(0);
      valve_fuel_pre.set_valve(0);
      valve_ox_pre.set_valve(0);
      SET_STATE(SHUTDOWN);
      shutdown_time = millis();
      break;
  }
}

void run_control(){
  long run_time = millis() - start_time - COUNTDOWN_DURATION;
  SEND(run_time, run_time);

  handle_igniter();

  #if CONFIGURATION == MK_2_FULL || CONFIGURATION == MK_2_LOW
    if (state!=STAND_BY  &&  state!=COOL_DOWN  &&  millis() > heartbeat_time + HEARTBEAT_TIMEOUT){
      Serial.println(F("Loss of data link"));
      abort_autosequence();
    }else
  #endif

  switch (state){
    case STAND_BY:
      if (!sensor_status){
        set_lcd_status("Sensor Failure");
      }
      break;

    case TERMINAL_COUNT:
      #if CONFIGURATION != DEMO
        if (!sensor_status){
          Serial.println(F("Sensor failure"));
          abort_autosequence();
        }else
      #endif
      if (run_time >= PRESTAGE_PREP_TIME){
        valve_fuel_pre.set_valve(1);
        valve_n2_choke.set_valve(1);
        valve_ox_pre.set_valve(1);
        SET_STATE(PRESTAGE_READY);
      }
      break;

    case PRESTAGE_READY:
      if (run_time >= PRESTAGE_TIME){
        fire_igniter();
        SET_STATE(PRESTAGE);
      }
      break;

    case PRESTAGE:
      if (run_time >= MAINSTAGE_TIME){
        valve_fuel_main.set_valve(1);
        valve_ox_main.set_valve(1);
        SET_STATE(MAINSTAGE);
      }
      break;

    case MAINSTAGE:
      #if CONFIGURATION != DEMO
        if (!sensor_status){
          Serial.println(F("Sensor Failure"));
          abort_autosequence();
        }else
      #endif
      if (outlet_temp >= MAX_COOLANT_TEMP){
        Serial.println(F("Temperature reached critical level. Shuttung down."));
        abort_autosequence();
      }
      else if (run_time >= THRUST_CHECK_TIME && force < MIN_THRUST){
        Serial.println(F("Thrust below critical level. Shutting down."));
        abort_autosequence();
      }

      if (run_time >= RUN_TIME){
        SET_STATE(OXYGEN_SHUTDOWN);
        valve_ox_pre.set_valve(0);
        shutdown_time = millis();
      }
      break;

    case OXYGEN_SHUTDOWN:
      if (millis() >= shutdown_time + OX_LEADTIME){
        valve_n2_choke.set_valve(0);
        valve_fuel_pre.set_valve(0);
        SET_STATE(SHUTDOWN);
      }
      break;

    case SHUTDOWN:
      if (millis() >= shutdown_time + PRE_LEADTIME){
        valve_ox_main.set_valve(0);
        valve_fuel_main.set_valve(0);
        valve_n2_drain.set_valve(1);
        SET_STATE(COOL_DOWN);
      }
      break;

    case COOL_DOWN:
      if (!valve_n2_drain.set_valve()){
        valve_n2_drain.set_valve(1);
      }
      if (millis() - shutdown_time >= COOLDOWN_TIME){
        Serial.println(F("Run finished"));
        valve_n2_drain.set_valve(0);
        SET_STATE(STAND_BY);
        start_time = 0;
      }
      break;
  }
}
