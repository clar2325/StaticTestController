// TODO: fill these in
void fuel_safety(bool open) {
  if (open)
    Serial.println("Opening fuel safety valve");
  else
    Serial.println("Closing fuel safety valve");
    
  SEND(fuel_safety, open);
}

void oxy_safety(bool open) {
  if (open)
    Serial.println("Opening oxygen safety valve");
  else
    Serial.println("Closing oxygen safety valve");
  SEND(oxy_safety, open);
}

// TODO send read settings instead of set points
void fuel_throttle(int setting) {
  Serial.print("Fuel control valve at ");
  Serial.println(setting);
  SEND(fuel_control, setting); 
}

void oxy_throttle(int setting) {
  Serial.print("Oxygen control valve at ");
  Serial.println(setting);
  SEND(oxy_control, setting);
}

void fire_ignitor() {
  Serial.println("Firing ignitor");
}

