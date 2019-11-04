#define DEMO 0
#define MK_1 1
#define MK_2 2

// Change this line to set configuration
#define CONFIGURATION MK_1

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
#define THERMO1_LED 31
#define THERMO2_LED 33
#define THERMO3_LED 35
#define ACCEL_LED 30
#define FORCE_LED 28
#define PRESSURE_FUEL_LED 36
#define PRESSURE_OX_LED 50
#define INLET_TEMP_LED 48
#define OUTLET_TEMP_LED 46
#define STATE_LED 38
#define STATUS_LED 44

// Accelerometer
Adafruit_MMA8451 mma;
int accel_error = 0;

// Analog Temperature Setup
#define INLET_TEMP A5
#define OUTLET_TEMP A4
#define NUMBER_OF_TEMP_SENSORS 2
int temp_error[NUMBER_OF_TEMP_SENSORS] = {0,0};

// Pressure Setup
#define PRESSURE_FUEL A4
#define PRESSURE_OX A3
#define PRESSURE_NUM_HIST_VALS 10
#define NUMBER_OF_PRESSURE_SENSORS 2

// Pressure sensor 0 = Fuel, Pressure sensor 1 = Oxidizer
float pressure_hist_vals[NUMBER_OF_PRESSURE_SENSORS][PRESSURE_NUM_HIST_VALS];
int pressure_val_num = 0;
bool pressure_zero_ready = false;
float pressure_zero_val[NUMBER_OF_PRESSURE_SENSORS] = {0,0};
int pressure_error[NUMBER_OF_PRESSURE_SENSORS] = {0,0};

// Thermocouple setup for MK_2
#if CONFIGURATION == MK_2
#define MAXDO   36
#define MAXCLK  38
#define MAXCS1  30
#define MAXCS2  32
#define MAXCS3  34
Adafruit_MAX31855 chamber_thermocouple_1(MAXCLK, MAXCS1, MAXDO);
Adafruit_MAX31855 chamber_thermocouple_2(MAXCLK, MAXCS2, MAXDO);
Adafruit_MAX31855 chamber_thermocouple_3(MAXCLK, MAXCS3, MAXDO);
#define NUMBER_OF_THERMOCOUPLES 3
int thermocouple_error[NUMBER_OF_THERMOCOUPLES] = {0,0,0};
#endif

// Load cell setup
#define LOAD_CELL_DOUT 2
#define LOAD_CELL_CLK  26
HX711 scale;
int force_error = 0;

// Sensor data
#if CONFIGURATION == MK_2
float chamber_temp[NUMBER_OF_THERMOCOUPLES];
#endif
float pressure_fuel, pressure_ox, force, x,y,z, inlet_temp, outlet_temp;

bool sensor_status = true;

// Engine controls
typedef enum {
  FUEL_PRE,
  FUEL_MAIN,
  OX_PRE,
  OX_MAIN
} valve_t;

bool valve_status[] = {false, false, false, false};

char data[10] = "";
char data_name[20] = "";

void setup() {
  // Initialize LED pins to be outputs
#if CONFIGURATION == MK_2
  pinMode(THERMO1_LED, OUTPUT);
  pinMode(THERMO2_LED, OUTPUT);
  pinMode(THERMO3_LED, OUTPUT);
#endif
  pinMode(ACCEL_LED, OUTPUT);
  pinMode(FORCE_LED, OUTPUT);
  pinMode(INLET_TEMP_LED, OUTPUT);
  pinMode(OUTLET_TEMP_LED, OUTPUT);
  pinMode(PRESSURE_FUEL_LED, OUTPUT);
  pinMode(PRESSURE_OX_LED, OUTPUT);
  pinMode(STATE_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  // Initially turn on all the LEDs
#if CONFIGURATION == MK_2
  digitalWrite(THERMO1_LED, HIGH);
  digitalWrite(THERMO2_LED, HIGH);
  digitalWrite(THERMO3_LED, HIGH);
#endif
  digitalWrite(ACCEL_LED, HIGH);
  digitalWrite(FORCE_LED, HIGH);
  digitalWrite(INLET_TEMP_LED, HIGH);
  digitalWrite(OUTLET_TEMP_LED, HIGH);
  digitalWrite(PRESSURE_FUEL_LED, HIGH);
  digitalWrite(PRESSURE_OX_LED, HIGH);
  digitalWrite(STATE_LED, HIGH);
  digitalWrite(STATUS_LED, HIGH);

  // Initialize serial
  while (!Serial);
  Serial.begin(115200);
  Serial.println(F("Initializing..."));
  
  // wait for chips to stabilize
  delay(500);
  
  // Initialize Pressure Sensors
  pinMode(PRESSURE_FUEL, INPUT);
  pinMode(PRESSURE_OX, INPUT);
  
  //Initialize Analog Temp Sensors
  pinMode(INLET_TEMP, INPUT);
  pinMode(OUTLET_TEMP, INPUT);
  
  // Initialize thermocouples for Mk.2
  #if CONFIGURATION == MK_2
  init_thermocouple("Chamber 1", THERMO1_LED, chamber_thermocouple_1);
  init_thermocouple("Chamber 2", THERMO2_LED, chamber_thermocouple_2);
  init_thermocouple("Chamber 3", THERMO3_LED, chamber_thermocouple_3);  
  #endif
  
  // Initialize load cell
  init_force(FORCE_LED, scale);
  
  // Initialize accelerometer
  Wire.begin();
  init_accelerometer(ACCEL_LED, mma);

  // Initialize engine controls
  init_engine();
  
  Serial.println("Setup Complete");
}

void loop() {
  // Grab force data
  force = read_force(FORCE_LED, scale, force_error); // Force is measured in lbs
  
  // Grab thermocouple data for Mk.2
  #if CONFIGURATION == MK_2
  chamber_temp[0] = read_thermocouple("Chamber 1", THERMO1_LED, chamber_thermocouple_1, thermocouple_error[0]);
  chamber_temp[1] = read_thermocouple("Chamber 2", THERMO2_LED, chamber_thermocouple_2, thermocouple_error[1]);
  chamber_temp[2] = read_thermocouple("Chamber 3", THERMO3_LED, chamber_thermocouple_3, thermocouple_error[2]);
  #endif
  
  // Grab pressure data
  pressure_fuel = read_pressure("Fuel", PRESSURE_FUEL_LED, PRESSURE_FUEL, pressure_error[0]);
  pressure_ox = read_pressure("Oxygen", PRESSURE_OX_LED, PRESSURE_OX, pressure_error[0]);
  
  // Update pressure tare data
  pressure_hist_vals[0][pressure_val_num] = pressure_fuel;
  pressure_hist_vals[1][pressure_val_num] = pressure_ox;
  
  // Tare pressures
  pressure_fuel -= pressure_zero_val[0];
  pressure_ox -= pressure_zero_val[1];
  
  pressure_val_num++;
  if (pressure_val_num >= PRESSURE_NUM_HIST_VALS) {
    pressure_val_num = 0;
    pressure_zero_ready = true;
  }
  
  // Grab analog temperature data
  inlet_temp = read_temp("Inlet", INLET_TEMP_LED, INLET_TEMP, temp_error[0]);
  outlet_temp = read_temp("Outlet", OUTLET_TEMP_LED, OUTLET_TEMP, temp_error[1]);
  
  // Grab accelerometer data (acceleration is measured in m/s^2)
  sensors_vec_t accel = read_accelerometer(ACCEL_LED, mma, accel_error);
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
  SEND_ITEM(fuel_pressure, pressure_fuel)
  SEND_ITEM(ox_pressure, pressure_ox)   
  #if CONFIGURATION == MK_2
  SEND_ITEM(chamber_temperature_1, chamber_temp[0])
  SEND_ITEM(chamber_temperature_2, chamber_temp[2])
  SEND_ITEM(chamber_temperature_3, chamber_temp[3])
  #endif
  SEND_ITEM(sensor_status, sensor_status)
  END_SEND

  // Read a command
  bool valve_command;
  
  BEGIN_READ
  READ_FLAG(zero_force) {
    Serial.println(F("Zeroing load cell"));
    scale.tare(); // Load Cell, Assuming there is no weight on the scale, reset to 0
  }
  
  READ_FLAG(zero_pressure) {
    if (pressure_zero_ready) {
      Serial.println(F("Zeroing fuel pressure"));
      pressure_zero_val[0] = mean(pressure_hist_vals[0], PRESSURE_NUM_HIST_VALS);
      Serial.println(F("Zeroing oxidizer pressure"));
      pressure_zero_val[1] = mean(pressure_hist_vals[1], PRESSURE_NUM_HIST_VALS);
      pressure_zero_ready = false;
      pressure_val_num = 0;
    }
    else
      Serial.println(F("Pressure zero values not ready"));
  }
  
  READ_FLAG(reset) {
    Serial.println(F("Resetting board"));
    asm volatile("  jmp 0"); // Perform a software reset of the board, reinitializing sensors
  }
  READ_FLAG(start) {
    start_countdown();
  }
  READ_FLAG(stop) {
    Serial.println(F("Manual abort initiated"));
    abort_autosequence();
  }
  READ_FLAG(fire_igniter) {
    fire_igniter();
  }
  READ_FIELD(fuel_pre_command, "%d", valve_command) {
    set_valve(FUEL_PRE, valve_command);
  }
  READ_FIELD(fuel_main_command, "%d", valve_command) {
    set_valve(FUEL_MAIN, valve_command);
  }
  READ_FIELD(ox_pre_command, "%d", valve_command) {
    set_valve(OX_PRE, valve_command);
  }
  READ_FIELD(ox_main_command, "%d", valve_command) {
    set_valve(OX_MAIN, valve_command);
  }
  READ_DEFAULT(data_name, data) {
    Serial.print(F("Invalid data field recieved: "));
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ
}
