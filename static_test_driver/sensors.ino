
#define SENSOR_ERROR_LIMIT 5 // Max number of errors in a row before deciding a sensor is faulty

#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_MIN_VALID -100
#define PRESSURE_MAX_VALID 1000

#define TEMP_MIN_VALID -10
#define TEMP_MAX_VALID 120

#define FORCE_MIN_VALID -50
#define FORCE_MAX_VALID 500

#if CONFIGURATION == MK_2
#define LOAD_CELL_CALIBRATION_FACTOR 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#else
#define LOAD_CELL_CALIBRATION_FACTOR 20400
#endif

#define LOAD_CELL_RETRY_INTERVAL 10
#define LOAD_CELL_MAX_RETRIES 20

String sensor_errors = "";

float mean(const float *data, unsigned int size) {
  float result = 0;
  for (int i = 0; i < size; i++)
      result += data[i];
  return result / size;
}

void update_sensor_errors() {
  set_lcd_errors(sensor_errors);
  sensor_errors = "";
}

void error_check(int &error, bool working, const String &sensor_type, const String &sensor_name="", const String &sensor_short_name="") {
  if (working) {
    error = 0;
  } else {
    if (sensor_errors.length()) {
      sensor_errors += ',';
    }
    sensor_errors += sensor_type.substring(0, min(sensor_type.length(), 2)) + sensor_short_name;
    if (!error) {
      Serial.print(sensor_name);
      if (sensor_name.length()) {
        Serial.print(' ');
      }
      Serial.print(sensor_type);
      Serial.println(F(" sensor error"));
    }
    error++;
    if (error > SENSOR_ERROR_LIMIT) {
      sensor_status = false;
    }
  }
}

float read_temp(int sensor, int &error, const String &sensor_name, const String &sensor_short_name) {
  float result = analogRead(sensor) * 5.0 * 100 / 1024;
  error_check(error, result > TEMP_MIN_VALID && result < TEMP_MAX_VALID, "temp", sensor_name, sensor_short_name);
  return result;
}

float read_pressure(int sensor, int &error, const String &sensor_name, const String &sensor_short_name) {
  float result = (analogRead(sensor) * 5 / 1024.0) * PRESSURE_CALIBRATION_FACTOR - PRESSURE_OFFSET;
  error_check(error, result > PRESSURE_MIN_VALID && result < PRESSURE_MAX_VALID, sensor_name, "pressure");
  return result;
}

void init_accelerometer(Adafruit_MMA8451 &mma) {
  bool working = false;
  if (mma.begin()) {
    mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 4 8)
    Serial.print(F("Accelerometer range ")); 
    Serial.print(2 << mma.getRange()); 
    Serial.println("G");
    working = true;
  }
  int error = 0;
  error_check(error, working, "accelerometer");
  delay(100);
}

sensors_vec_t read_accelerometer(Adafruit_MMA8451 &mma, int &error) {
  sensors_event_t event;
  mma.getEvent(&event);
  sensors_vec_t accel = event.acceleration;
  error_check(error, !(mma.x == -1 && mma.y == -1 && mma.z == -1), "accelerometer");
  return accel;
}

void init_force(HX711 &scale) {
  scale.begin(LOAD_CELL_DOUT, LOAD_CELL_CLK);
  
  // Calibrate load cell
  scale.set_scale(LOAD_CELL_CALIBRATION_FACTOR); // This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); // Load Cell, Assuming there is no weight on the scale, reset to 0

  // Try reading a value from the load cell
  int error = 0;
  read_force(scale, error);
  
  if (!error) {
    Serial.println(F("Load cell amp connected"));
  }
  delay(100);
}

float read_force(HX711 &scale, int &error) {
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
  error_check(error, is_ready && !isnan(result) && result > FORCE_MIN_VALID && result < FORCE_MAX_VALID, "Force");
  return result;
}

void init_thermocouple(Adafruit_MAX31855 &thermocouple, const String &sensor_name, const String &sensor_short_name) {
  int error = 0;
  thermocouple.begin();
  read_thermocouple(thermocouple, error, sensor_name, sensor_short_name);
  if (!error) {
    Serial.print(sensor_name);
    Serial.println(F(" theromocouple connected"));
  }
  delay(100);
}

float read_thermocouple(Adafruit_MAX31855 &thermocouple, int &error, const String &sensor_name, const String &sensor_short_name) {
  float result = thermocouple.readCelsius();
  error_check(error, !isnan(result) && result > 0, "thermocouple", sensor_name, sensor_short_name);
  return result;
}
