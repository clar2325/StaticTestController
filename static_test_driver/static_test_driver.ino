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
#define THERMO1_LED 28
#define THERMO2_LED 30
#define THERMO3_LED 32
#define FORCE_LED 42
#define PRESSURE_FUEL_LED 44
#define PRESSURE_OX_LED 50 
#define INLET_TEMP_LED 46 
#define OUTLET_TEMP_LED 48 
#define STATE_LED 38 //TODO: Discard STATE_LED code for current sensors revision due to pin conflict. Pin # was changed to satisfy next revision
#define STATUS_LED 40 //TODO: Discare STATUS_LED code for current sensors revision due to pin conflict. Pin # was changed to satisfy next revision

// Accelerometer
Adafruit_MMA8451 mma;

// Analog Temperature Setup
#define INLET_TEMP A4
#define OUTLET_TEMP A5

// Pressure Setup
#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_FUEL A2
#define PRESSURE_OX A3 
#define PRESSURE_NUM_HIST_VALS 10
#define NUMBER_OF_PRESSURE_SENSORS 2
#define NUMBER_OF_THERMOCOUPLES 3

// Pressure sensor 0 = Fuel, Pressure sensor 1 = Oxidizer
float pressure_hist_vals [NUMBER_OF_PRESSURE_SENSORS][PRESSURE_NUM_HIST_VALS];
int pressure_val_num = 0;
bool pressure_zero_ready = false;
float pressure_zero_val[NUMBER_OF_PRESSURE_SENSORS]= {0,0};

// Thermocouple setup for MK_2
#if CONFIGURATION == MK_2
#define MAXDO   24
#define MAXCLK  52
#define MAXCS1  22
#define MAXCS2  33
#define MAXCS3  31
Adafruit_MAX31855 chamber_thermocouple_1(MAXCLK, MAXCS1, MAXDO);
Adafruit_MAX31855 chamber_thermocouple_2(MAXCLK, MAXCS2, MAXDO);
Adafruit_MAX31855 chamber_thermocouple_3(MAXCLK, MAXCS3, MAXDO);
#endif

// Load cell setup
#if CONFIGURATION == MK_2
#define LOAD_CELL_CALIBRATION_FACTOR 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#else
#define LOAD_CELL_CALIBRATION_FACTOR 1067141
#endif

#define LOAD_CELL_DOUT 2
#define LOAD_CELL_CLK  26
HX711 scale(LOAD_CELL_DOUT, LOAD_CELL_CLK);

// Sensor data
float chamber_temp[NUMBER_OF_THERMOCOUPLES], pressure_fuel, pressure_ox, force, x,y,z, inlet_temp, outlet_temp;

#define SENSOR_ERROR_LIMIT 5 // Max number of errors in a row before deciding a sensor is faulty

int chamber_temp_error[NUMBER_OF_THERMOCOUPLES] = {0,0,0};
int pressure_error[NUMBER_OF_PRESSURE_SENSORS] = {0,0};
int force_error = 0;
int accel_error = 0;
bool sensor_status = true;

// Engine control setup
#define FUEL_PRE    0
#define FUEL_MAIN   1
#define OXY_PRE     2
#define OXY_MAIN    3

bool valve_status[] = {false, false, false, false};

uint8_t valve_pins[] = {37, 36, 39, 34};

const char *valve_names[] = {"Fuel prestage", "Fuel mainstage", "Oxygen prestage", "Oxygen mainstage"};
const char *valve_telemetry_ids[] = {"fuel_pre_setting", "fuel_main_setting", "oxy_pre_setting", "oxy_main_setting"};

char data[10] = "";
char data_name[20] = "";

#define IGNITER_PIN 15

void setup() {
  // Initialize LED pins to be outputs
  pinMode(THERMO1_LED, OUTPUT);
  pinMode(THERMO2_LED, OUTPUT);
  pinMode(THERMO3_LED, OUTPUT);
  pinMode(INLET_TEMP_LED, OUTPUT);
  pinMode(OUTLET_TEMP_LED, OUTPUT);
  pinMode(FORCE_LED, OUTPUT);
  pinMode(PRESSURE_FUEL_LED, OUTPUT);
  pinMode(PRESSURE_OX_LED, OUTPUT);
  pinMode(STATE_LED, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  // Initialize serial
  while (!Serial);
  Serial.begin(115200);
  Serial.println(F("Initializing..."));
  
  // wait for MAX chips to stabilize
  delay(500);
  
  //Initialize Pressure Sensors
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
  
  // Calibrate load cell
  scale.set_scale(LOAD_CELL_CALIBRATION_FACTOR); // This value is obtained by using the SparkFun_HX711_Calibration sketch
  
  // Initialize accelerometer
  Wire.begin();
  init_accelerometer(mma, STATE_LED);// Should be pin 13

  // Initialize engine controls
  for (uint8_t i = 0; i < sizeof(valve_pins); i++) {
    pinMode(valve_pins[i], OUTPUT);
  }
  pinMode(IGNITER_PIN, OUTPUT);
  Serial.println("Setup Complete");
}

void loop() {
  // Grab force data
  // TODO: This hangs when the load cell amp isn't connected. Figure out a way to check this first.
  force = scale.get_units(); // Force is measured in lbs
  
  // TODO: Error checking for load cell?
  
  // Grab thermocouple data for Mk.2
  #if CONFIGURATION == MK_2
  chamber_temp[0] = read_thermocouple("Chamber 1", THERMO1_LED, chamber_thermocouple_1, chamber_temp_error[0]);
  chamber_temp[1] = read_thermocouple("Chamber 2", THERMO2_LED, chamber_thermocouple_2, chamber_temp_error[1]);
  chamber_temp[2] = read_thermocouple("Chamber 3", THERMO3_LED, chamber_thermocouple_3, chamber_temp_error[2]);
  //Serial.println("Collected Thermocouple Data");
  #endif
  
  
  // Grab pressure data
  pressure_fuel = (analogRead(PRESSURE_FUEL) * 5 / 1024.0) * PRESSURE_CALIBRATION_FACTOR - (PRESSURE_OFFSET + pressure_zero_val[0]); // Pressure is measured in PSIG
  pressure_ox = (analogRead(PRESSURE_OX) * 5 / 1024.0) * PRESSURE_CALIBRATION_FACTOR - (PRESSURE_OFFSET + pressure_zero_val[1]); // Pressure is measured in PSIG
  
  // TODO: Error checking

  // Update pressure tare data
  pressure_hist_vals[0][pressure_val_num] = pressure_fuel;
  pressure_hist_vals[1][pressure_val_num] = pressure_ox;
  
  
  pressure_val_num++;
  if (pressure_val_num >= PRESSURE_NUM_HIST_VALS) {
    pressure_val_num = 0;
    pressure_zero_ready = true;
  }
  
  // Grab analog temperature data
  inlet_temp = (analogRead(INLET_TEMP) * 5.0 / 1024.0 - 0.5) * 100 ;
  outlet_temp = (analogRead(OUTLET_TEMP) * 5.0 / 1024.0 - 0.5) * 100 ;
  
  //TODO: Add error checking for temp?
  
  // Grab accelerometer data (acceleration is measured in m/s^2)
  //sensors_vec_t accel = read_accelerometer(mma, accel_error);
  //x=accel.x;  y=accel.y;  z=accel.z;
  //commented out for now, accelermometer data being buggy
  
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
  //SEND_ITEM(pressure_fuel, pressure_fuel)
  //SEND_ITEM(pressure_oxidizer, pressure_ox)   
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
  
  //TODO: Update flag names to match the following code 
  READ_FLAG(zero_pressure_fuel) {
    if (pressure_zero_ready) {
      Serial.print(F("Zeroing fuel pressure"));
      pressure_zero_val[0] = mean(pressure_hist_vals[0], PRESSURE_NUM_HIST_VALS);
    }
    else
      Serial.println(F("Fuel pressure zero value not ready"));
  }
  
  READ_FLAG(zero_pressure_ox) {
    if (pressure_zero_ready) {
      Serial.print(F("Zeroing oxidizer pressure"));
      pressure_zero_val[1] = mean(pressure_hist_vals[1], PRESSURE_NUM_HIST_VALS);
      pressure_zero_ready = false;
    }
    else
      Serial.println(F("Oxidizer pressure zero value not ready"));
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
  READ_FIELD(fuel_pre_command, "%d", valve_command) {
    //set_valve(FUEL_PRE, valve_command);
  }
  READ_FIELD(fuel_main_command, "%d", valve_command) {
    //set_valve(FUEL_MAIN, valve_command);
  }
  READ_FIELD(oxy_pre_command, "%d", valve_command) {
    //set_valve(OXY_PRE, valve_command);
  }
  READ_FIELD(oxy_main_command, "%d", valve_command) {
    //set_valve(OXY_MAIN, valve_command);
  }
  READ_DEFAULT(data_name, data) {
    Serial.print(F("Invalid data field recieved: "));
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ
}
