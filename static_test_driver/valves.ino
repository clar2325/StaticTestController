
/*
const uint8_t valve_pins[] = {34, 33, 32, 31};

const char *valve_names[] = {"Fuel prestage", "Fuel mainstage", "Oxygen prestage", "Oxygen mainstage"};
const char *valve_telemetry_ids[] = {"fuel_pre_setting", "fuel_main_setting", "ox_pre_setting", "ox_main_setting"};


This function will have to be remade in the static_test_driver file. After we create a Valve array etc etc.
void init_engine() {
  for (uint8_t i = 0; i < sizeof(valve_pins); i++) {
    pinMode(valve_pins[i], OUTPUT);
  }
  pinMode(IGNITER_PIN, OUTPUT);
}
*/
#include "defs.h"



// valve initializer function
void Valve::init_valve() {
  m_current_state = false;
  pinMode(m_valvepin, OUTPUT); 
  digitalWrite(m_valvepin, m_current_state);
}

// valve setting function
void Valve::set_valve(bool setting) {
  m_current_state = setting;
  Serial.print(m_valvename);
  Serial.print(F(" to "));
  Serial.println(m_current_state? F("open") : F("closed"));
  SEND_NAME(m_telemetry_id, m_current_state);
  digitalWrite(m_valvepin, m_current_state);
}
