int fuel_target = 0;
int oxy_target = 0;

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
  int fuel_setting = 0;
  int oxy_setting = 0;
  
  SEND(fuel_setting, fuel_setting);
  SEND(oxy_setting, oxy_setting);

  // TODO: write to relays to control throttle valves
}

void fire_ignitor() {
  Serial.println("Firing ignitor");
}
