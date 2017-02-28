#define DEMO 0
#define MK_1 1
#define MK_2 2

// Change this line to set configuration
#define CONFIGURATION MK_2

#if !(CONFIGURATION == DEMO || CONFIGURATION == MK_1 || CONFIGURATION == MK_2)
#error "Invalid configuration value"
#endif

#include <SPI.h>
#include <Wire.h>
#include <HX711.h> // https://github.com/bogde/HX711
#include <Adafruit_MMA8451.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_Sensor.h>
#include <Telemetry.h>
#include <avr/pgmspace.h>

// Accelerometer
Adafruit_MMA8451 mma;

// Thermocouple Setup
#define MAXDO   9
#define MAXCLK  12

#define MAXCS1  5
#define MAXCS2  11
Adafruit_MAX31855 inlet_thermocouple(MAXCLK, MAXCS1, MAXDO);
Adafruit_MAX31855 outlet_thermocouple(MAXCLK, MAXCS2, MAXDO);

// Thermocouple and pressure setup for MK_2
#if CONFIGURATION == MK_2
#define MAXCS3  47
Adafruit_MAX31855 chamber_thermocouple(MAXCLK, MAXCS3, MAXDO);

#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_PIN A1

#define PRESSURE_NUM_HIST_VALS 10

float pressure_hist_vals[PRESSURE_NUM_HIST_VALS];
int pressure_val_num = 0;
bool pressure_zero_ready = false;
float pressure_zero_val = 0;
#endif

// Force Setup
#define calibration_factor 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#define DOUT  15
#define CLK  14
HX711 scale(DOUT, CLK);

// Sensor data
float chamber_temp, inlet_temp, outlet_temp, pressure, force, x,y,z;

#define SENSOR_ERROR_LIMIT 5 // Max number of errors in a row before deciding a sensor is faulty

int chamber_temp_error = 0;
int inlet_temp_error = 0;
int outlet_temp_error = 0;
int pressure_error = 0;
int force_error = 0;
int accel_error = 0;
bool sensor_status = true;

// Engine control setup
int fuel_target = 0;
int oxy_target = 0;
int fuel_setting = 0;
int oxy_setting = 0;

char data[10] = "";
char data_name[20] = "";

void setup() {
  while (!Serial);
  Serial.begin(230400);
  Serial.println(F("Initializing..."));
  // wait for MAX chips to stabilize
  delay(500);
  //--------------------Set up thermocouple and pressure sensor for MK_2--------------------
  #if CONFIGURATION == MK_2
  init_thermocouple("Chamber", chamber_thermocouple);
  
  pinMode(PRESSURE_PIN, INPUT);
  #endif
  
  //------------------Set up thermocouples-------------------------------
  init_thermocouple("Inlet", inlet_thermocouple);
  init_thermocouple("Outlet", outlet_thermocouple);
  
  // Calibrate load cell
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  
   //-------------set up accelerometer---------------
  if (!mma.begin()) {
    Serial.println(F("Accelerometer error"));
    sensor_status = false;
  }
  else {
    mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 5 8)
    Serial.print(F("Accelerometer range ")); 
    Serial.print(2 << mma.getRange()); 
    Serial.println("G");
  }
  delay(100);
  
  Wire.begin();
}

void loop() {
  //--------------Grab Force Data----------------------
  force = scale.get_units(); // Force is measured in lbs
  // TODO: Error checking
  
  //--------------Grab Temp Data for MK_2
  #if CONFIGURATION == MK_2
  chamber_temp = read_thermocouple("Chamber", chamber_thermocouple, chamber_temp_error);
  
  //---------------Grab Pressure Data-------------------
  pressure = (analogRead (PRESSURE_PIN)* 5/ 1024.0) * PRESSURE_CALIBRATION_FACTOR - (PRESSURE_OFFSET + pressure_zero_val); // Pressure is measured in PSIG
  // TODO: Error checking

  // Update pressure tare data
  pressure_hist_vals[pressure_val_num] = pressure;
  pressure_val_num++;
  if (pressure_val_num >= PRESSURE_NUM_HIST_VALS) {
    pressure_val_num = 0;
    pressure_zero_ready = true;
  }
  #endif
  
  // --------------Grab Tempdata------------------------ 
  inlet_temp = read_thermocouple("Inlet", inlet_thermocouple, inlet_temp_error);
  outlet_temp = read_thermocouple("Outlet", outlet_thermocouple, outlet_temp_error);
  
  //----------Grab Acc Data (acceleration is measured in m/s^2)
  sensors_event_t event;
  mma.getEvent(&event); // TODO: Error checking
  x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;

  // Run autonomous control
  run_control();

  BEGIN_SEND
  SEND_ITEM(force, force)
  SEND_ITEM(acceleration, x)
  SEND_GROUP_ITEM(y)
  SEND_GROUP_ITEM(z)
  SEND_ITEM(outlet_temperature, outlet_temp)
  SEND_ITEM(inlet_temperature, inlet_temp)
  #if CONFIGURATION == MK_2
  SEND_ITEM(chamber_temperature, chamber_temp)
  SEND_ITEM(pressure,pressure)
  #endif
  SEND_ITEM(sensor_status, sensor_status)
  END_SEND
  
  int fuel_command, oxy_command;
  
  BEGIN_READ
  READ_FLAG(zero_force) {
    Serial.println(F("Zeroing load cell"));
    scale.tare(); // Load Cell, Assuming there is no weight on the scale, reset to 0
  }
  READ_FLAG(zero_pressure) {
    if (pressure_zero_ready) {
      Serial.println(F("Zeroing pressure"));
      pressure_zero_ready = false;
      pressure_zero_val = 0;
      for (int i = 0; i < PRESSURE_NUM_HIST_VALS; i++)
        pressure_zero_val += pressure_hist_vals[i];
      pressure_zero_val /= PRESSURE_NUM_HIST_VALS;
    }
    else {
      Serial.println(F("Zero value not ready"));
    }
  }
  READ_FLAG(reset) {
    Serial.println(F("Resetting board"));
    asm volatile("  jmp 0"); // Perform a software reset of the board, reinitializing sensors
  }
  READ_FLAG(start) {
    start_countdown();
  }
  READ_FLAG(stop) {
    abort_autosequence();
  }
  READ_FIELD(fuel_command, "%d", fuel_command) {
    fuel_throttle(fuel_command);
  }
  READ_FIELD(oxy_command, "%d", oxy_command) {
    oxy_throttle(oxy_command);
  }
  READ_DEFAULT(data_name, data) {
    Serial.print(F("Invalid data field recieved: "));
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ
}
