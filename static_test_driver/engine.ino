

void set_valve(valve_t valve, bool setting) {
  Serial.print(valve_names[valve]);
  Serial.print(" at ");
  Serial.println(setting);
  SEND_NAME(valve_telemetry_ids[valve], setting);
  valve_status[valve] = setting;
  digitalWrite(valve_pins[valve], setting);
}

void fire_ignitor() {
  Serial.println("Firing ignitor");
  digitalWrite(IGNITOR_PIN, 1);
}

void reset_ignitor() {
  Serial.println("Reset ignitor");
  digitalWrite(IGNITOR_PIN, 0);
}
