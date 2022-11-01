#include "defs.h"

String sensor_errors = "";

//Number of Sensors 
//Load Cells - 4
//Pressure Transducers - 4
//Thermocouples - n/a (not a priority)
//Accelerometer - probably 1 so no need to work on that too much.

//-------------------------------------------------------------------------------------------
//Utility Functions
//-------------------------------------------------------------------------------------------


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
      sensor_status = false; //static_test_driver
    }

   
  }
}

//-------------------------------------------------------------------------------------------
//LoadCell 
//-------------------------------------------------------------------------------------------

//SENSOR DEVICE 1 

void LoadCell::init_loadcell() {
  m_scale.begin(m_dout, m_clk);
  
  // Calibrate load cell
  m_scale.set_scale(m_calibrationFactor); // This value is obtained by using the SparkFun_HX711_Calibration sketch
  //Calibration is just the slope of the data function we get from the cell, we use it to find force at the values we get.
  m_scale.tare(); // Load Cell, Assuming there is no weight on the scale, reset to 0 mark intial valeue of cell as 0

  // Try reading a value from the load cell
  read_force();
  
  if (!m_error) {
    Serial.println(F("Load cell amp connected"));
  }
  delay(100);
}


float LoadCell::read_force() {
  // Wait for load cell data to become ready
  bool is_ready = false;
  for (unsigned int i = 0; i < LOAD_CELL_MAX_RETRIES; i++) {
    if (m_scale.is_ready()) {
      is_ready = true;
      break;
    }
    delay(LOAD_CELL_RETRY_INTERVAL);
  }

  // Read a value from the load cell
  float result = 0;
  if (is_ready) {
    result = m_scale.get_units();
  }
  error_check(m_error, is_ready && !isnan(result) && result > FORCE_MIN_VALID && result < FORCE_MAX_VALID, "Force");
  return result;
}


//-------------------------------------------------------------------------------------------
//Thermocouple 
//-------------------------------------------------------------------------------------------

//SENSOR DEVICE 2


float Thermocouple::read_temp() {
  float result = analogRead(m_thermocouplepin) * 5.0 * 100 / 1024;
  error_check(m_error, result > TEMP_MIN_VALID && result < TEMP_MAX_VALID, "temp", m_sensor_name, m_sensor_short_name);
  return result;
}

void Thermocouple::init_thermocouple() {

  pinMode(m_thermocouplepin, INPUT);
  
  int error = 0;
  m_thermocouple.begin();
  read_thermocouple();
  if (!m_error) {
    Serial.print(m_sensor_name);
    Serial.println(F(" theromocouple connected"));
  }
  delay(100);
}

float Thermocouple::read_thermocouple() {
  float result = m_thermocouple.readCelsius();
  error_check(m_error, !isnan(result) && result > 0, "thermocouple", m_sensor_name, m_sensor_short_name);
  return result;
}



//-------------------------------------------------------------------------------------------
//Pressure Transducers
//-------------------------------------------------------------------------------------------

//Sensor Device 3 


void PressureTransducer::init_transducer()
{
  pinMode(m_pressurepin, INPUT);
}

float PressureTransducer::read_pressure() {
  float result = (analogRead(m_pressurepin) * 5 / 1024.0) * PRESSURE_CALIBRATION_FACTOR - PRESSURE_OFFSET;
  error_check(m_error, result > PRESSURE_MIN_VALID && result < PRESSURE_MAX_VALID, m_sensor_name, "pressure");
  return result;
}

void PressureTransducer::updatePressures()
{
   m_current_pressure = read_pressure();

    if(m_current_hist_val >= PRESSURE_NUM_HIST_VALS)
    {
      m_current_hist_val = 0;
      m_zero_ready = true;
    }
    
    m_pressure_history[m_current_hist_val] = m_current_pressure;
    m_current_pressure -= m_tare;
      m_current_hist_val++;
}


//-------------------------------------------------------------------------------------------
//Accelerometer
//-------------------------------------------------------------------------------------------

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
