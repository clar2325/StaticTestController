
void init_thermocouple(const char *sensor_name, Adafruit_MAX31855 &thermocouple) {
  float result = chamber_thermocouple.readCelsius();
  if (isnan(result) || result == 0) {
    Serial.print(sensor_name);
    Serial.println(F(" temp sensor error"));
    sensor_status = false;
  }
  else {
    Serial.print(sensor_name);
    Serial.println(F(" temp sensor connected"));
  }
  delay(100);
}

float read_thermocouple(const char *sensor_name, Adafruit_MAX31855 &thermocouple, int &error) {
  float result = thermocouple.readCelsius();
  if (isnan(result) || result == 0) {
    if (!error) {
      Serial.print(sensor_name);
      Serial.println(F(" temp sensor error"));
    }
    error++;
  }
  else {
    error = 0;
  }
  if (error > SENSOR_ERROR_LIMIT) {
    sensor_status = false;
  }
  return result;
}

