#include "communications.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
//#include <SD.h>
#include <Wire.h>
//#include "RTClib.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#define NUM_TEMP_SENSORS 2
#define ONE_WIRE_BUS 2                              // Data wire is plugged into pin 2 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);                      // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);                // Pass our oneWire reference to Dallas Temperature.
//File dataFile;                                      // tempFile,accFile; 
//RTC_DS1307 RTC;
Adafruit_MMA8451 mma = Adafruit_MMA8451();
const int chipSelect=10;                            // Use digital pin 10 as the slave select pin (SPI bus).
float temp[NUM_TEMP_SENSORS]={0, 0},x=0,y=0,z=0;

// This is the information on the sensor being used. 
// See the www.vernier.com/products/sensors.

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
char filename[] = "DATA000.csv";

void setup() {
  //------------- set up temp sensor-----------
  Serial.begin(9600);
  Serial.println("Initializing...");
  sensors.begin();

  byte addr[8][NUM_TEMP_SENSORS];
  for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
    oneWire.search(addr[i]); // address on 1wire bus
  
    oneWire.reset();             // rest 1-Wire
    oneWire.select(addr[i]);     // select DS18B20
  
    oneWire.write(0x4E);         // write on scratchPad
    oneWire.write(0x00);         // User byte 0 - Unused
    oneWire.write(0x00);         // User byte 1 - Unused
    oneWire.write(0x1F);         // set up 9 bits (0x1F)
  
    oneWire.reset();             // reset 1-Wire
  }
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  //-------------set up accelerometer---------------
  if (!mma.begin()) {
    Serial.println("Acc error");
    //while (1); // Commented out for testing purpouses
  }
  
  //mma.setRange(MMA8451_RANGE_2_G);  // set acc range (2 5 8)
  Serial.print("Acc range "); Serial.print(2 << mma.getRange()); Serial.println("G");
  
  Wire.begin();
}

unsigned long loops = 0;
void loop() {
  loops++;
  
  float count = analogRead(A0);
  float voltage = count / 1023 * 5.0;// convert from count to raw voltage
  float force_reading = intercept + voltage * slope; //converts voltage to sensor reading

  prev_vals[val_num] = force_reading;
  val_num++;
  if (val_num >= NUM_HIST_VALS) {
    val_num = 0;
    zero_ready = true;
  }


  // --------------Grab Tempdata------------------------ 
  // Why "byIndex"? You can have more than one IC on the same bus. 

  // Temp sensors are slow, so alternate taking data and sending a request each loop
  if (loops % 2) {
    for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
      temp[i] = sensors.getTempCByIndex(i);
    }
    SEND(inlet_temperature,  temp[0])
    SEND(outlet_temperature, temp[1])    
  }
  else {
    // Send the command to get temperatures for the next loop
    sensors.requestTemperatures();
  }
  
  /* ---Get a new sensor event */ 
  sensors_event_t event;
  mma.getEvent(&event);

  /*---------- Display the results (acceleration is measured in m/s^2) */ 
  
  x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;
  //storeData(force_reading, tempc, x, y, z);

  /* Run autonoumous control */
  // Get a throttle setting, throttle the engine
  
  run_control();
  
  SEND(force, force_reading)
  SEND(x, x)
  SEND(y, y)
  SEND(z, z)

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
    start_autosequence();
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
