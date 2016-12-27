int fuel_target;
int oxy_target;

void fuel_throttle(int setting) {
  Serial.print("Fuel control valve at ");
  Serial.println(setting);
  SEND(fuel_target, setting);
  fuel_target = setting;
}

void oxy_throttle(int setting) {
  Serial.print("Oxygen control valve at ");
  Serial.println(setting);
  SEND(oxy_target, setting);
  oxy_target = setting;
}

// TODO
void update_throttle() {
  // TODO: Read fuel and oxygen valve settings from potentiometers
  int fuel_setting;
  int oxy_setting;
  
  SEND(fuel_setting, fuel_setting);
  SEND(oxy_setting, oxy_setting);

  // TODO: write to relays to control throttle valves
}

void fire_ignitor() {
  Serial.println("Firing ignitor");
}
