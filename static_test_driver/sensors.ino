float mean(const float *data, unsigned int size) {
  float result = 0;
  for (int i = 0; i < size; i++)
      result += data[i];
  return result / size;
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
