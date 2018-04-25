

void set_valve(int valve, bool setting) {
  Serial.print(valve_names[valve]);
  Serial.print(" at ");
  Serial.println(setting);
  SEND_NAME(valve_telemetry_ids[valve], setting);
  valve_status[valve] = setting;
  digitalWrite(valve_pins[valve], setting);
}

void fire_igniter() {
  Serial.println("Firing igniter");
  digitalWrite(IGNITER_PIN, 1);
}

void reset_igniter() {
  Serial.println("Reset igniter");
  digitalWrite(IGNITER_PIN, 0);
}
