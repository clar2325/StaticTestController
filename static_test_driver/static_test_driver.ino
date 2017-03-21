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

// LEDs
#define THERMO1_LED 23
#define THERMO2_LED 25
#define THERMO3_LED 27
#define FORCE_LED 29
#define PRESSURE_LED 31
#define STATE_LED 33
#define STATUS_LED 35

// Accelerometer
Adafruit_MMA8451 mma;

// Thermocouple Setup
#define MAXDO   50
#define MAXCLK  52

#define MAXCS1  43
#define MAXCS2  30
Adafruit_MAX31855 inlet_thermocouple(MAXCLK, MAXCS1, MAXDO);
Adafruit_MAX31855 outlet_thermocouple(MAXCLK, MAXCS2, MAXDO);

// Thermocouple and pressure setup for MK_2
#if CONFIGURATION == MK_2
#define MAXCS3  32
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
#define FORCE_CALIBRATION_FACTOR 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#define FORCE_DOUT 32
#define FORCE_CLK  26
HX711 scale(FORCE_DOUT, FORCE_CLK);

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
  // Initialize LED pins to be outputs
  pinMode(THERMO1_LED, OUTPUT);
  pinMode(THERMO2_LED, OUTPUT);
  pinMode(THERMO3_LED, OUTPUT);
  pinMode(FORCE_LED, OUTPUT);
  pinMode(PRESSURE_LED, OUTPUT);
  pinMode(STATE_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  // Initialize serial
  while (!Serial);
  Serial.begin(230400);
  Serial.println(F("Initializing..."));
  
  // wait for MAX chips to stabilize
  delay(500);
  
  // Set up thermocouple and pressure sensor for Mk.2
  #if CONFIGURATION == MK_2
  init_thermocouple("Chamber", THERMO3_LED, chamber_thermocouple);
  
  pinMode(PRESSURE_PIN, INPUT);
  #endif
  
  // Set up thermocouples
  init_thermocouple("Inlet", THERMO1_LED, inlet_thermocouple);
  init_thermocouple("Outlet", THERMO2_LED, outlet_thermocouple);
  
  // Calibrate load cell
  //scale.set_scale(FORCE_CALIBRATION_FACTOR); // This value is obtained by using the SparkFun_HX711_Calibration sketch
  
  // Set up accelerometer
  init_accelerometer(mma, STATE_LED);// Should be pin 13
  
  Wire.begin();
}

void loop() {
  // Grab force data
  force = scale.get_units(); // Force is measured in lbs
  // TODO: Error checking
  
  // Grab thermocouple data for Mk.2
  #if CONFIGURATION == MK_2
  chamber_temp = read_thermocouple("Chamber", THERMO3_LED, chamber_thermocouple, chamber_temp_error);
  
  // Grab pressure data
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
  
  // Grab thermocouple data
  inlet_temp = read_thermocouple("Inlet", THERMO1_LED, inlet_thermocouple, inlet_temp_error);
  outlet_temp = read_thermocouple("Outlet", THERMO2_LED, outlet_thermocouple, outlet_temp_error);
  
  // Grab accelerometer data (acceleration is measured in m/s^2)
  sensors_vec_t accel = read_accelerometer(mma, accel_error);
  x=accel.x;  y=accel.y;  z=accel.z;

  // Run autonomous control
  run_control();

  // Send collected data
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

  // Read a command
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
