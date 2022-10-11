
const uint8_t valve_pins[] = {34, 33, 32, 31,-,-,-,-};

const char *valve_names[] = {"Fuel prestage", "Fuel mainstage", "Oxygen prestage", "Oxygen mainstage", "Nitrogen prestage", "Nitrogen mainstage", "Kerovent prestage", "Kerovent mainstage"};
const char *valve_telemetry_ids[] = {"fuel_pre_setting", "fuel_main_setting", "ox_pre_setting", "ox_main_setting", "N_pre_setting", "N_main_setting", "Kv_pre_setting", "Kv_main_setting"};

void init_engine() {
  for (uint8_t i = 0; i < sizeof(valve_pins); i++) {
    pinMode(valve_pins[i], OUTPUT);
  }
  pinMode(IGNITER_PIN, OUTPUT);
}

void set_valve(int valve, bool setting) {
  Serial.print(valve_names[valve]);
  Serial.print(F(" to "));
  Serial.println(setting? F("open") : F("closed"));
  SEND_NAME(valve_telemetry_ids[valve], setting);
  valve_status[valve] = setting;
  digitalWrite(valve_pins[valve], setting);
}
