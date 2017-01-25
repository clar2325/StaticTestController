
#include <SPI.h>
//#include <SD.h>
#include <Wire.h>
//#include "RTClib.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_MAX31855.h>
#include <Adafruit_Sensor.h>
#include <Telemetry.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();
const int chipSelect=10;                            // Use digital pin 10 as the slave select pin (SPI bus).
float chamber_temp, inlet_temp, outlet_temp,pressure,x,y,z;
bool temp_status = false;
#define PROTOTYPE_SENSORS

//Thermocouple pins
#define MAXDO1   3
#define MAXCS1   4
#define MAXCLK1  5
Adafruit_MAX31855 Chamber_Thermocouple(MAXCLK1, MAXCS1, MAXDO1);

#ifndef PROTOTYPE_SENSORS
#define MAXDO2   6
#define MAXCS2   7
#define MAXCLK2  8
#define MAXDO3   9
#define MAXCS3   10
#define MAXCLK3  11
Adafruit_MAX31855 Inlet_Thermocouple(MAXCLK2, MAXCS2, MAXDO2);
Adafruit_MAX31855 Outlet_Thermocouple(MAXCLK3, MAXCS3, MAXDO3);
#endif

//Pressure Setup
#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_PIN 1


#define HIGH_RANGE

#ifdef HIGH_RANGE
float intercept = -250;
float slope = 250;
#else
float intercept = -1000;
float slope = 1000;
#endif
int TimeBetweenReadings = 500; // in ms
int ReadingNumber=0;

#define NUM_HIST_VALS 10

float prev_vals[NUM_HIST_VALS];
int val_num = 0;
bool zero_ready = false;

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
  //--------------------Set up 3 thermocouples--------------------
  temp = Chamber_Thermocouple.readCelsius();
  if (isnan(temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  #ifndef PROTOTYPE_SENSORS
  temp = Inlet_Thermocouple.readCelsius();
  if (isnan(temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  temp = Outlet_Thermocouple.readCelsius();
  if (isnan(temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  #endif
  //-------------------Set up pressure sensors--------------------
  pinMode (PRESSURE_PIN,INPUT);
  unsigned long loops = 0;
}


void loop() {

  //--------------Grab Force Data----------------------
  float count = analogRead(A0);
  float voltage = count / 1023 * 5.0;// convert from count to raw voltage
  float force_reading = intercept + voltage * slope; //converts voltage to sensor reading

  prev_vals[val_num] = force_reading;
  val_num++;
  if (val_num >= NUM_HIST_VALS) {
    val_num = 0;
    zero_ready = true;
  }

  //---------------Grab Pressure Data-------------------
  pressure = (analogRead (PRESSURE_PIN)* 5/ 1024.) * PRESSURE_CALIBRATION_FACTOR - PRESSURE_OFFSET;
  

  // --------------Grab Tempdata------------------------ 

  // Temp sensors are slow, so alternate taking data from each sensor each loop
  // Why "byIndex"? You can have more than one IC on the same bus. 
  chamber_temp = Chamber_Thermocouple.readCelsius();
  if (isnan(chamber_temp)){
    Serial.println("Temp sensor err");
    temp_status = false;
  } 
  else {
    Serial.println("Temp sensor connected");
    temp_status = true;
  }
  #ifndef PROTOTYPE_SENSORS
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
  #endif
  //---------- Display the results (acceleration is measured in m/s^2)
  
  //x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;
  //storeData(force_reading, tempc, x, y, z);

  // Run autonoumous control
  // Get a throttle setting, throttle the engine
  
  run_control();

  BEGIN_SEND
  SEND_ITEM(force, force_reading)
  SEND_ITEM(pressure,pressure)
  //SEND_ITEM(acceleration, x)
  //SEND_GROUP_ITEM(y)
  //SEND_GROUP_ITEM(z)
  #ifndef PROTOTYPE SENSORS  
  SEND_ITEM(outlet_temperature, outlet_temp)
  SEND_ITEM(inlet_temperature, inlet_temp)
  #endif
  SEND_ITEM(chamber_temperature, chamber_temp)
  END_SEND

  BEGIN_READ
  READ_FLAG(zero) {
    if (zero_ready) {
      Serial.println("Zeroing");
      float zero_val = 0;
      for (int i = 0; i < NUM_HIST_VALS; i++)
        zero_val += prev_vals[i];
      zero_val /= NUM_HIST_VALS;
      intercept -= force_reading;
    }
    else {
      Serial.println("Zero err");
    }
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
