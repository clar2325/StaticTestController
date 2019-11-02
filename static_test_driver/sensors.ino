
#define SENSOR_ERROR_LIMIT 5 // Max number of errors in a row before deciding a sensor is faulty

#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33

#if CONFIGURATION == MK_2
#define LOAD_CELL_CALIBRATION_FACTOR 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#else
#define LOAD_CELL_CALIBRATION_FACTOR 1067141
#endif

#define LOAD_CELL_RETRY_INTERVAL 10
#define LOAD_CELL_MAX_RETRIES 20

float mean(const float *data, unsigned int size) {
  float result = 0;
  for (int i = 0; i < size; i++)
      result += data[i];
  return result / size;
}

void error_check(const char *sensor_name, const char *sensor_type, int led, int &error, bool working) {
  if (working) {
    error = 0;
    digitalWrite(led, HIGH);
  } else {
    if (!error) {
      Serial.print(sensor_name);
      Serial.print(' ');
      Serial.print(sensor_type);
      Serial.println(F(" sensor error"));
    }
    error++;
    if (error > SENSOR_ERROR_LIMIT) {
      sensor_status = false;
      digitalWrite(STATUS_LED, false);
    }
    digitalWrite(led, LOW);
  }
}

float read_temp(const char *sensor_name, int led, int sensor, int &error) {
  float result = analogRead(sensor) * 5.0 * 100 / 1024;
  error_check(sensor_name, "temp", led, error, result > 0 && result < 100);
  return result;
}

float read_pressure(const char *sensor_name, int led, int sensor, int &error) {
  float result = (analogRead(PRESSURE_FUEL) * 5 / 1024.0) * PRESSURE_CALIBRATION_FACTOR - PRESSURE_OFFSET;
  error_check(sensor_name, "pressure", led, error, result > 0 && result < 150);
  return result;
}

void init_thermocouple(const char *sensor_name, int led, Adafruit_MAX31855 &thermocouple) {
  int error = 0;
  read_thermocouple(sensor_name, led, thermocouple, error);
  if (!error) {
    Serial.print(sensor_name);
    Serial.println(F(" theromocouple connected"));
    digitalWrite(led, HIGH);
  }
  delay(100);
}

float read_thermocouple(const char *sensor_name, int led, Adafruit_MAX31855 &thermocouple, int &error) {
  float result = thermocouple.readCelsius();
  error_check(sensor_name, "thermocouple", led, error, !isnan(result) && result > 0);
  return result;
}

void init_accelerometer(int led, Adafruit_MMA8451 &mma) {
  bool working = false;
  if (mma.begin()) {
    mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 4 8)
    Serial.print(F("Accelerometer range ")); 
    Serial.print(2 << mma.getRange()); 
    Serial.println("G");
    working = true;
  }
  int error = 0;
  error_check("Accelerometer", "", led, error, working);
  delay(100);
}

sensors_vec_t read_accelerometer(int led, Adafruit_MMA8451 &mma, int &error) {
  sensors_event_t event;
//  mma.getEvent(&event);
  sensors_vec_t accel = event.acceleration;
  error_check("Accelerometer", "", led, error, abs(accel.x) < 2 && abs(accel.y) < 2 && abs(accel.z) < 2);
  return accel;
}

void init_force(int led, HX711 &scale) {
  scale.begin(LOAD_CELL_DOUT, LOAD_CELL_CLK);
  
  // Calibrate load cell
  scale.set_scale(LOAD_CELL_CALIBRATION_FACTOR); // This value is obtained by using the SparkFun_HX711_Calibration sketch

  // Try reading a value from the load cell
  int error = 0;
  read_force(led, scale, error);
  
  if (!error) {
    Serial.println(F("Load cell amp connected"));
    digitalWrite(led, HIGH);
  }
  delay(100);
}

float read_force(int led, HX711 &scale, int &error) {
  // Wait for load cell data to become ready
  bool is_ready = false;
  for (unsigned i = 0; i < LOAD_CELL_MAX_RETRIES; i++) {
    if (scale.is_ready()) {
      is_ready = true;
      break;
    }
    delay(LOAD_CELL_RETRY_INTERVAL);
  }

  // Read a value from the load cell
  float result = 0;
  if (is_ready) {
    result = scale.get_units();
  }
  error_check("Force", "", led, error, is_ready && !isnan(result) && result > 0);
  return result;
}
