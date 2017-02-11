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
Adafruit_MMA8451 mma = Adafruit_MMA8451();

// Thermocouple and pressure setup for MK_2
#if CONFIGURATION == MK_2
#define MAXDO1   3
#define MAXCS1   4
#define MAXCLK1  5
Adafruit_MAX31855 chamber_thermocouple(MAXCLK1, MAXCS1, MAXDO1);
#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_PIN A1
#endif

// Thermocouple Setup
#define MAXDO2   6
#define MAXCS2   7
#define MAXCLK2  8
#define MAXDO3   9
#define MAXCS3   11
#define MAXCLK3  12
Adafruit_MAX31855 inlet_thermocouple(MAXCLK2, MAXCS2, MAXDO2);
Adafruit_MAX31855 outlet_thermocouple(MAXCLK3, MAXCS3, MAXDO3);

// Force Setup
#define calibration_factor 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#define DOUT  15
#define CLK  14
HX711 scale(DOUT, CLK);

// Sensor data
float chamber_temp, inlet_temp, outlet_temp, pressure, force, x,y,z;
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
  chamber_temp = chamber_thermocouple.readCelsius();
  if (isnan(chamber_temp) || chamber_temp == 0) {
    Serial.println(F("Chamber Temp sensor error"));
    sensor_status = false;
  }
  else {
    Serial.println(F("Chamber Temp sensor connected"));
  }
  delay(100);
  
  pinMode(PRESSURE_PIN, INPUT);
  #endif
  
  //------------------Set up thermocouples-------------------------------
  inlet_temp = inlet_thermocouple.readCelsius();
  if (isnan(inlet_temp) || inlet_temp == 0) {
    Serial.println(F("Inlet Temp sensor error"));
    sensor_status = false;
  } 
  else {
    Serial.println(F("Inlet Temp sensor connected"));
    Serial.println(inlet_temp);
  }
  delay(100);
  
  outlet_temp = outlet_thermocouple.readCelsius();
  if (isnan(outlet_temp) || outlet_temp == 0) {
    Serial.println(F("Outlet Temp sensor error"));
    sensor_status = false;
  }
  else {
    Serial.println(F("Outlet Temp sensor connected"));
  }
  delay(100);
  
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
  force = scale.get_units(); //Force is measured in lbs
  
  //--------------Grab Temp Data for MK_2
  #if CONFIGURATION == MK_2
  chamber_temp = chamber_thermocouple.readCelsius();
  if (sensor_status && (isnan(chamber_temp) || chamber_temp == 0)) {
    Serial.println(F("Chamber Temp sensor error"));
    sensor_status = false;
  }
  //---------------Grab Pressure Data-------------------
  pressure = (analogRead (PRESSURE_PIN)* 5/ 1024.0) * PRESSURE_CALIBRATION_FACTOR - PRESSURE_OFFSET; // Pressure is measured in PSIG
  #endif
  
  // --------------Grab Tempdata------------------------ 
  inlet_temp = inlet_thermocouple.readCelsius();
  if (sensor_status && (isnan(inlet_temp) || inlet_temp == 0)) {
    Serial.println(F("Inlet Temp sensor error"));
    sensor_status = false;
  }
  outlet_temp = outlet_thermocouple.readCelsius();
  if (sensor_status && (isnan(outlet_temp) || outlet_temp == 0)) {
    Serial.println(F("Outlet Temp sensor error"));
    sensor_status = false;
  }
  
  //----------Grab Acc Data (acceleration is measured in m/s^2)
  sensors_event_t event;
  mma.getEvent(&event);
  x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;

  // Run autonoumous control
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
  READ_FLAG(zero) {
    scale.tare(); // Load Cell, Assuming there is no weight on the scale at start up, reset the scale to 0
  }
  READ_FLAG(reset) {
    asm volatile ("  jmp 0"); // Perform a software reset of the board, reinitializing sensors
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
