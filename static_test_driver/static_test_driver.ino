
#include <SPI.h>
#include <Wire.h>
#include <HX711.h> // https://github.com/bogde/HX711
#include <Adafruit_MMA8451.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LiquidCrystal.h>
#include <Telemetry.h>
#include <avr/pgmspace.h>
#include "defs.h"

//declare pressure transducers
PressureTransducer pressure_fuel{PRESSURE_FUEL, "fuel", "Fl"};
PressureTransducer pressure_ox{PRESSURE_OX, "oxygen", "ox"};
PressureTransducer pressure_fuel_injector{PRESSURE_FUEL_INJECTOR, "injector fuel", "FlE"};
PressureTransducer pressure_ox_injector{PRESSURE_OX_INJECTOR, "injector oxygen", "OxE"};

bool global_pressure_zero_ready = false;
//declare load cells

LoadCell loadcell_1{LOAD_CELL_1_DOUT, LOAD_CELL_1_CLK};
LoadCell loadcell_2{LOAD_CELL_2_DOUT, LOAD_CELL_2_CLK};
LoadCell loadcell_3{LOAD_CELL_3_DOUT, LOAD_CELL_3_CLK};
LoadCell loadcell_4{LOAD_CELL_4_DOUT, LOAD_CELL_4_CLK};


// LCD
// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0);


//declare thermocouples 
Thermocouple thermocouple_1{THERMOCOUPLE_PIN_1, "temperature", "temp"};
Thermocouple thermocouple_2{THERMOCOUPLE_PIN_2, "temperature", "temp"};


bool sensor_status = true;

//declare all valves
Valve valve_fuel_pre{FUEL_PRE_PIN, "fuel pre", "FlPre"};
Valve valve_fuel_main{FUEL_MAIN_PIN, "fuel main", "FlMain"};
Valve valve_ox_pre{OX_PRE_PIN, "oxygen pre", "OxPre"};
Valve valve_ox_main{OX_MAIN_PIN, "oxygen main", "OxMain"};
Valve valve_n2_choke{N2_CHOKE_PIN, "n2 choke", "N2Pre"};
Valve valve_n2_drain{N2_DRAIN_PIN, "n2 drain", "N2Main"};

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
    Serial.println(F("Mk 2 static test driver"));
    Serial.println(F("Initializing..."));
    delay(500); // wait for chips to stabilize

    //init forces and pressure
    pressure_fuel.init_transducer();
    pressure_ox.init_transducer();
    pressure_fuel_injector.init_transducer();
    pressure_ox_injector.init_transducer();

    loadcell_1.init_loadcell();
    loadcell_2.init_loadcell();
    loadcell_3.init_loadcell();
    loadcell_4.init_loadcell();
   
    //thermocouples

    thermocouple_1.init_thermocouple();
    thermocouple_2.init_thermocouple();
   
    //init all valves
    valve_fuel_pre.init_valve();
    valve_fuel_main.init_valve();
    valve_ox_pre.init_valve();
    valve_ox_main.init_valve();
    valve_n2_choke.init_valve();
    valve_n2_drain.init_valve();

    // Set initial state
    init_autosequence();

    Serial.println("Setup Complete");
}

void loop() {
    // Grab force data
    loadcell_1.updateForces();
    loadcell_2.updateForces();
    loadcell_3.updateForces();
    loadcell_4.updateForces();
    
    // Update pressures
    pressure_fuel.updatePressures();
    pressure_ox.updatePressures();
    pressure_fuel_injector.updatePressures();
    pressure_ox_injector.updatePressures();
    
    // Grab thermocouple data 
    thermocouple_1.updateTemps();
    thermocouple_2.updateTemps();
   
    // Update sensor diagnostic message on LCD
    update_sensor_errors();

    // Run autonomous control
    run_control();

    // Send collected data
  
    BEGIN_SEND
        SEND_ITEM(force, loadcell_1.m_current_force)
        SEND_ITEM(force, loadcell_2.m_current_force)
        SEND_ITEM(force, loadcell_3.m_current_force)
        SEND_ITEM(force, loadcell_4.m_current_force)
        //Don't think I can send force data to separate locations right now
        
        //SEND_ITEM(outlet_temp, outlet_temp)
        //SEND_ITEM(inlet_temp, inlet_temp)
        
        SEND_ITEM(fuel_press, pressure_fuel.m_current_pressure)
        SEND_ITEM(ox_press, pressure_ox.m_current_pressure)
        SEND_ITEM(fuel_inj_press, pressure_fuel_injector.m_current_pressure)
        SEND_ITEM(ox_inj_press, pressure_ox_injector.m_current_pressure)
        //send thermocouple data
        //What keywords to use?
        
    SEND_ITEM(sensor_status, sensor_status)
        END_SEND

        // Read a command
        bool valve_command;

    BEGIN_READ
        READ_FLAG(zero_force) {
        Serial.println(F("Zeroing load cells"));
        loadcell_1.zeroForces(); 
        loadcell_2.zeroForces(); 
        loadcell_3.zeroForces(); 
        loadcell_4.zeroForces(); 
    }

    READ_FLAG(zero_pressure) {
        if (pressure_fuel.m_zero_ready || pressure_ox.m_zero_ready) {
            Serial.println(F("Zeroing fuel pressure"));
            pressure_fuel.zeroPressures();
            pressure_fuel.m_zero_ready = false;
            Serial.println(F("Zeroing oxidizer pressure"));
            pressure_ox.zeroPressures();
            pressure_ox.m_zero_ready = false;
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
       valve_fuel_pre.set_valve(valve_command);
    }
    READ_FIELD(fuel_main_command, "%d", valve_command) {
        valve_fuel_pre.set_valve(valve_command);
    }
    READ_FIELD(ox_pre_command, "%d", valve_command) {
        valve_fuel_pre.set_valve(valve_command);
    }
    READ_FIELD(ox_main_command, "%d", valve_command) {
        valve_fuel_pre.set_valve(valve_command);
    }
    READ_DEFAULT(data_name, data) {
        Serial.print(F("Invalid data field recieved: "));
        Serial.print(data_name);
        Serial.print(":");
        Serial.println(data);
    }
    END_READ
}
