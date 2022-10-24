#define DEMO 0
#define MK_2 2

// Change this line to set configuration
#define CONFIGURATION MK_2

#if !(CONFIGURATION == DEMO || CONFIGURATION == MK_2)
#error "Invalid configuration value"
#endif

#include <SPI.h>
#include <Wire.h>
#include <HX711.h> // https://github.com/bogde/HX711
#include <Adafruit_MMA8451.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LiquidCrystal.h>
#include <Telemetry.h>
#include <avr/pgmspace.h>

// Accelerometer
Adafruit_MMA8451 mma;
int accel_error = 0;

// Analog Temperature Setup
#define INLET_TEMP A5
#define OUTLET_TEMP A4
#define NUMBER_OF_TEMP_SENSORS 4
int temp_error[NUMBER_OF_TEMP_SENSORS] = {0,0};

// Igniter pin assignment
uint8_t IGNITER_PIN = 15;

// Valve pin assignment
uint8_t FUEL_PRE_PIN = 34;
uint8_t FUEL_MAIN_PIN = 33;
uint8_t OX_PRE_PIN = 32;
uint8_t OX_MAIN_PIN = 31;
uint8_t N2_CHOKE_PIN = 30;
uint8_t N2_DRAIN_PIN = 29;

// Pressure Setup
#define PRESSURE_FUEL A0
#define PRESSURE_OX A1
#define PRESSURE_FUEL_INJECTOR A2
#define PRESSURE_OX_INJECTOR A3
#define PRESSURE_NUM_HIST_VALS 10
#define NUMBER_OF_PRESSURE_SENSORS 4

// Pressure sensor 0 = Fuel, Pressure sensor 1 = Oxidizer
float pressure_hist_vals[NUMBER_OF_PRESSURE_SENSORS][PRESSURE_NUM_HIST_VALS];
int pressure_val_num = 0;
bool pressure_zero_ready = false;
float pressure_zero_val[NUMBER_OF_PRESSURE_SENSORS] = {0,0,0,0};
int pressure_error[NUMBER_OF_PRESSURE_SENSORS] = {0,0,0,0};

// Load cell setup
#define LOAD_CELL_DOUT 2
#define LOAD_CELL_CLK  26
HX711 scale;
int force_error = 0;

// Thermocouple setup for MK_2
#if CONFIGURATION == MK_2
#define NUMBER_OF_THERMOCOUPLES 3
// Using hardware SPI port: MISO=50, MOSI=51, SCK=52
Adafruit_MAX31855 chamber_thermocouples[NUMBER_OF_THERMOCOUPLES] = {
  Adafruit_MAX31855(45), Adafruit_MAX31855(46), Adafruit_MAX31855(47) // Pins 45-49 reserved for CS
};
int thermocouple_error[NUMBER_OF_THERMOCOUPLES] = {0,0,0};
#endif

// LCD
// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0);

// Sensor data
#if CONFIGURATION == MK_2
float chamber_temp[NUMBER_OF_THERMOCOUPLES];
#endif
float pressure_fuel, pressure_ox, pressure_fuel_injector, pressure_ox_injector, force, x,y,z, inlet_temp, outlet_temp;

bool sensor_status = true;

// Engine controls
typedef enum {
  FUEL_PRE,
  FUEL_MAIN,
  OX_PRE,
  OX_MAIN,
  N2_CHOKE,
  N2_DRAIN
} valve_t;

bool valve_status[] = {false, false, false, false, false, false};

unsigned long last_heartbeat = 0;

// Generally-used variables for parsing commands
char data[10] = "";
char data_name[20] = "";

// Calling this performs a software reset of the board, reinitializing sensors
void (*reset)(void) = 0;

void setup() {
  // Initialize LCD, set up the number of rows and columns
  lcd.begin(16, 2);
  set_lcd_status("Initializing...");

  // Initialize serial
  while (!Serial);
  Serial.begin(115200);
#if CONFIGURATION == DEMO
  Serial.println(F("Demo static test driver"));
#elif CONFIGURATION == MK_2
  Serial.println(F("Mk 2 static test driver"));
#endif
  Serial.println(F("Initializing..."));

  // wait for chips to stabilize
  delay(500);

  // Initialize Pressure Sensors
  pinMode(PRESSURE_FUEL, INPUT);
  pinMode(PRESSURE_OX, INPUT);
  pinMode(PRESSURE_FUEL_INJECTOR, INPUT);
  pinMode(PRESSURE_OX_INJECTOR, INPUT);

  // Initialize Analog Temp Sensors
  pinMode(INLET_TEMP, INPUT);
  pinMode(OUTLET_TEMP, INPUT);

  // Initialize load cell
  init_force(scale);

  // Initialize accelerometer
  Wire.begin();
  init_accelerometer(mma);

  // Initialize thermocouples for Mk.2
  #if CONFIGURATION == MK_2
  char thermo_name[] = "chamber n";
  char thermo_short_name[] = "n";
  for (unsigned i = 0; i < NUMBER_OF_THERMOCOUPLES; i++) {
    thermo_name[8] = thermo_short_name[0] = '1' + i;
    init_thermocouple(chamber_thermocouples[i], thermo_name, thermo_short_name);
  }
  #endif

  // Initialize valves
  init_valve(FUEL_PRE_PIN);
  init_valve(FUEL_MAIN_PIN);
  init_valve(OX_PRE_PIN);
  init_valve(OX_MAIN_PIN);
  init_valve(N2_CHOKE_PIN);
  init_valve(N2_DRAIN_PIN);

  // Initialize igniter
  init_igniter(IGNITER_PIN);

  // Set initial state
  init_autosequence();

  Serial.println("Setup Complete");
}

void loop() {
  // Grab force data
  force = read_force(scale, force_error); // Force is measured in lbs

  // Grab pressure data
  pressure_fuel = read_pressure(PRESSURE_FUEL, pressure_error[0], "fuel", "Fl");
  pressure_ox = read_pressure(PRESSURE_OX, pressure_error[1], "oxygen", "Ox");
  pressure_fuel_injector = read_pressure(PRESSURE_FUEL_INJECTOR, pressure_error[2], "injector fuel", "FlE");
  pressure_ox_injector = read_pressure(PRESSURE_OX_INJECTOR, pressure_error[3], "injector oxygen", "OxE");

  // Update pressure tare data
  pressure_hist_vals[0][pressure_val_num] = pressure_fuel;
  pressure_hist_vals[1][pressure_val_num] = pressure_ox;
  pressure_hist_vals[2][pressure_val_num] = pressure_fuel_injector;
  pressure_hist_vals[3][pressure_val_num] = pressure_ox_injector;

  // Tare pressures
  pressure_fuel -= pressure_zero_val[0];
  pressure_ox -= pressure_zero_val[1];
  pressure_fuel_injector -= pressure_zero_val[2];
  pressure_ox_injector -= pressure_zero_val[3];

  pressure_val_num++;
  if (pressure_val_num >= PRESSURE_NUM_HIST_VALS) {
    pressure_val_num = 0;
    pressure_zero_ready = true;
  }

  // Grab analog temperature data
  inlet_temp = read_temp(INLET_TEMP, temp_error[0], "inlet", "In");
  outlet_temp = read_temp(OUTLET_TEMP, temp_error[1], "outlet", "Out");

  // Grab accelerometer data (acceleration is measured in m/s^2)
  sensors_vec_t accel = read_accelerometer(mma, accel_error);
  x=accel.x;  y=accel.y;  z=accel.z;

  // Grab thermocouple data for Mk.2
  #if CONFIGURATION == MK_2
  char thermo_name[] = "chamber n";
  char thermo_short_name[] = "n";
  for (unsigned i = 0; i < NUMBER_OF_THERMOCOUPLES; i++) {
    thermo_name[8] = thermo_short_name[0] = '1' + i;
    chamber_temp[i] = read_thermocouple(chamber_thermocouples[i], thermocouple_error[i], thermo_name, thermo_short_name);
  }
  #endif

  // Update sensor diagnostic message on LCD
  update_sensor_errors();

  // Run autonomous control
  run_control();

  // Send collected data
  BEGIN_SEND
    SEND_ITEM(force, force)
    SEND_ITEM(accel, x)
    SEND_GROUP_ITEM(y)
    SEND_GROUP_ITEM(z)
    SEND_ITEM(outlet_temp, outlet_temp)
    SEND_ITEM(inlet_temp, inlet_temp)
    SEND_ITEM(fuel_press, pressure_fuel)
    SEND_ITEM(ox_press, pressure_ox)
    SEND_ITEM(fuel_inj_press, pressure_fuel_injector)
    SEND_ITEM(ox_inj_press, pressure_ox_injector)
    #if CONFIGURATION == MK_2
    char chamber_temp_item_name[] = "chamber_temp_n";
    for (unsigned i = 0; i < NUMBER_OF_THERMOCOUPLES; i++) {
      chamber_temp_item_name[13] = '1' + i;
      SEND_ITEM_NAME(chamber_temp_item_name, chamber_temp[i])
    }
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
    else {
      Serial.println(F("Pressure zero values not ready"));
    }
  }

  READ_FLAG(heartbeat) {
    heartbeat();
  }
  READ_FLAG(reset) {
    Serial.println(F("Resetting board"));
    reset();
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
  READ_FIELD(n2_choke_command, "%d", valve_command) {
    set_valve(N2_CHOKE, valve_command);
  }
  READ_FIELD(n2_drain_command, "%d", valve_command) {
    set_valve(N2_DRAIN, valve_command);
  }
  READ_DEFAULT(data_name, data) {
    Serial.print(F("Invalid data field recieved: "));
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ
}

