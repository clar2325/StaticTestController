#define DEMO 0
#define MK_1 1
#define MK_2 2

#include <SPI.h>
//#include <SD.h>
#include <Wire.h>
#include <HX711.h>
//#include "RTClib.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_Sensor.h>
#include <Telemetry.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();
const int chipSelect=10;                            // Use digital pin 10 as the slave select pin (SPI bus).
float chamber_temp, inlet_temp, outlet_temp,pressure,x,y,z;
bool temp_status = false;
#define CONFIGURATION DEMO

//Thermocouple and pressure setup for MK_2
#if CONFIGURATION == MK_2
#define MAXDO1   3
#define MAXCS1   4
#define MAXCLK1  5
Adafruit_MAX31855 Chamber_Thermocouple(MAXCLK1, MAXCS1, MAXDO1);
#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_PIN 1
#endif

//Thermocouple Setup
#define MAXDO2   6
#define MAXCS2   7
#define MAXCLK2  8
#define MAXDO3   9
#define MAXCS3   11
#define MAXCLK3  12
Adafruit_MAX31855 Inlet_Thermocouple(MAXCLK2, MAXCS2, MAXDO2);
Adafruit_MAX31855 Outlet_Thermocouple(MAXCLK3, MAXCS3, MAXDO3);

//Force Setup
#define calibration_factor 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#define DOUT  13
#define CLK  14
HX711 scale(DOUT, CLK);

int TimeBetweenReadings = 500; // in ms
int ReadingNumber=0;


char data[10] = "";
char data_name[20] = "";

// Logging
//char filename[] = "DATA000.csv";

//I commented the setup and loop functions so that I could made separate ones without all the hardware implementations
//They will be uncommented and tested after making sure the state machine is working correctly

/*void setup() {
  Serial.begin(230400);
  Serial.println("Initializing...");
  //------------- set up temp sensor-----------


  //-------------set up accelerometer---------------
  if (!mma.begin()) {
    Serial.println("Acc error");
    while(1){}
  }
  
  mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 5 8)
  Serial.print("Acc range "); Serial.print(2 << mma.getRange()); Serial.println("G");
  
  Wire.begin();
}*/

void setup()
{
  while (!Serial);
  Serial.begin(230400);
  Serial.println("Initializing...");
  // wait for MAX chip to stabilize
  delay(500);
  //--------------------Set up thermocouple and pressure sensor for MK_2--------------------
  #if CONFIGURATION == MK_2
  chamber_temp = Chamber_Thermocouple.readCelsius();
  if (isnan(chamber_temp)){
    Serial.println("Chamber Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Chamber Temp sensor connected");
    temp_status = true;
  }
  pinMode (PRESSURE_PIN,INPUT);
  #endif
  
  //------------------Set up thermocouples-------------------------------
  inlet_temp = Inlet_Thermocouple.readCelsius();
  if (isnan(inlet_temp)){
    Serial.println("Inlet Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Inlet Temp sensor connected");
    temp_status = true;
  }
  outlet_temp = Outlet_Thermocouple.readCelsius();
  if (isnan(outlet_temp)){
    Serial.println("Outlet Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Outlet Temp sensor connected");
    temp_status = true;
  }
  //Calibrate load cell
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  
   //-------------set up accelerometer---------------
  if (!mma.begin()) {
    Serial.println("Acc error");
    while(1){}
  }
  mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 5 8)
  Serial.print("Acc range "); 
  Serial.print(2 << mma.getRange()); 
  Serial.println("G");
  Wire.begin();

  unsigned long loops = 0;
}


void loop() {

  //--------------Grab Force Data----------------------
  force_reading = scale.get_units(); //Force is measured in lbs
  
  //--------------Grab Temp Data for MK_2
  #if CONFIGURATION == MK_2
  chamber_temp = Chamber_Thermocouple.readCelsius();
  if (isnan(chamber_temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  //---------------Grab Pressure Data-------------------
  pressure = (analogRead (PRESSURE_PIN)* 5/ 1024.) * PRESSURE_CALIBRATION_FACTOR - PRESSURE_OFFSET; //Pressure is measured in PSIG
  #endif
  
  // --------------Grab Tempdata------------------------ 
  inlet_temp = Inlet_Thermocouple.readCelsius();
  if (isnan(inlet_temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  outlet_temp = Outlet_Thermocouple.readCelsius();
  if (isnan(outlet_temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  //----------Grab Acc Data (acceleration is measured in m/s^2)
  x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;
  storeData(force_reading, tempc, x, y, z);

  // Run autonoumous control
  // Get a throttle setting, throttle the engine
  
  run_control();

  BEGIN_SEND
  SEND_ITEM(force, force_reading)
  SEND_ITEM(acceleration, x)
  SEND_GROUP_ITEM(y)
  SEND_GROUP_ITEM(z)
  SEND_ITEM(outlet_temperature, outlet_temp)
  SEND_ITEM(inlet_temperature, inlet_temp)
  #if CONFIGURATION == MK_2
  SEND_ITEM(chamber_temperature, chamber_temp)
  SEND_ITEM(pressure,pressure)
  #endif
  END_SEND

  BEGIN_READ
  READ_FLAG(zero) {
  scale.tare(); //Load Cell, Assuming there is no weight on the scale at start up, reset the scale to 0
  }
  READ_FLAG(start) {
    start_countdown();
  }
  READ_FLAG(stop) {
    abort_autosequence();
  }
  READ_DEFAULT(data_name, data) {
    Serial.print("Invalid: ");
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ

//  delay(10);
}
