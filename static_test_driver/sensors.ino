void tare_pressure (int pressure_pin) {
  int index;
  float pressure;
  if (pressure_pin == PRESSURE_FUEL) {
    index = 0;
    pressure = pressure_fuel;
  }
  else if (pressure_pin == PRESSURE_OX) {
    index = 1;
    pressure = pressure_ox;
  }
  else
    return;
  
  pressure_hist_vals[index][pressure_val_num[index]] = pressure;
  pressure_val_num[index]++;
  if (pressure_val_num[index] >= PRESSURE_NUM_HIST_VALS) {
    pressure_val_num[index] = 0;
    pressure_zero_ready[index] = true;
  }
}

void zero_pressure (int pressure_pin, const char *pressure_name){
  int index;
  if (pressure_pin == PRESSURE_FUEL) {
    index = 0;
  }
  else if (pressure_pin == PRESSURE_OX) {
    index = 1;
  }
  else
    return;
  
  if (pressure_zero_ready[index]) {
      Serial.print(F("Zeroing "));
      Serial.print(pressure_name);
      Serial.println(F(" pressure"));
      pressure_zero_ready[index] = false;
      pressure_zero_val[index] = 0;
      for (int i = 0; i < PRESSURE_NUM_HIST_VALS; i++)
        pressure_zero_val[index] += pressure_hist_vals[index][i];
      pressure_zero_val[index] /= PRESSURE_NUM_HIST_VALS;
    }
    else {
      Serial.print(pressure_name);
      Serial.println(F(" pressure zero value not ready"));
    }
}

void init_thermocouple(const char *sensor_name, int led, Adafruit_MAX31855 &thermocouple) {
  float result = thermocouple.readCelsius();
  if (isnan(result) || result == 0) {
    Serial.print(sensor_name);
    Serial.println(F(" temp sensor error"));
    sensor_status = false;
  }
  else {
    Serial.print(sensor_name);
    Serial.println(F(" temp sensor connected"));
    digitalWrite(led, HIGH);
  }
  delay(100);
}

float read_thermocouple(const char *sensor_name, int led, Adafruit_MAX31855 &thermocouple, int &error) {
  float result = thermocouple.readCelsius();
  if (isnan(result) || result == 0) {
    if (!error) {
      Serial.print(sensor_name);
      Serial.println(F(" temp sensor error"));
    }
    error++;
    digitalWrite(led, LOW);
  }
  else {
    error = 0;
    digitalWrite(led, HIGH);
  }
  if (error > SENSOR_ERROR_LIMIT) {
    sensor_status = false;
  }
  return result;
}

void init_accelerometer(Adafruit_MMA8451 &mma, int led) {
  if (!mma.begin()) {
    Serial.println(F("Accelerometer error"));
    sensor_status = false;
  }
  else {
    mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 5 8)
    Serial.print(F("Accelerometer range ")); 
    Serial.print(2 << mma.getRange()); 
    Serial.println("G");
    digitalWrite(led, HIGH);
  }
  delay(100);
}

sensors_vec_t read_accelerometer(Adafruit_MMA8451 &mma, int &error) {
  sensors_event_t event;
  mma.getEvent(&event); // TODO: Error checking
  return event.acceleration;
}
