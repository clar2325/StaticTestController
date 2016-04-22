#include "communications.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
//#include <SD.h>
#include <Wire.h>
//#include "RTClib.h"
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#define ONE_WIRE_BUS 2                              // Data wire is plugged into pin 2 on the Arduino
OneWire oneWire(ONE_WIRE_BUS);                      // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);                // Pass our oneWire reference to Dallas Temperature.
//File dataFile;                                      // tempFile,accFile; 
//RTC_DS1307 RTC;
Adafruit_MMA8451 mma = Adafruit_MMA8451();
const int chipSelect=10;                            // Use digital pin 10 as the slave select pin (SPI bus).
float tempc=0,x=0,y=0,z=0;

// This is the information on the sensor being used. 
// See the www.vernier.com/products/sensors.
/*
#define HIGH_RANGE

#ifdef HIGH_RANGE
float intercept = -250;
float slope = 250;
#else
float intercept = -1000;
float slope = 1000;
#endif
*/

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
  sensors.begin();

  byte addr[8];
  oneWire.search(addr); // address on 1wire bus

  oneWire.reset();             // rest 1-Wire
  oneWire.select(addr);        // select DS18B20

  oneWire.write(0x4E);         // write on scratchPad
  oneWire.write(0x00);         // User byte 0 - Unused
  oneWire.write(0x00);         // User byte 1 - Unused
  oneWire.write(0x1F);         // set up 9 bits (0x1F)

  oneWire.reset();             // reset 1-Wire
  //Serial.println("Temperature in degree celsius, ");

  //-------------set up accelerometer---------------
  if (!mma.begin()) {
    Serial.println("Acc error");
    while (1);
  }
  
  mma.setRange(MMA8451_RANGE_8_G);  // set acc range (2 5 8)
  Serial.print("Acc range "); Serial.print(2 << mma.getRange()); Serial.println("G");
   
  //--------------set up RTC-------------------  
   
  Wire.begin();
  /*RTC.begin();
  if (!RTC.isrunning()) {
    Serial.println("RTC err");
  }
  else {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }*/
    
  //--------set up SD card---------------
  /*
  pinMode(10,OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD err");
    return;
  } 
  else {
    for (uint16_t i = 0; i < 1000; i++) {
      filename[4] = i/100 + '0';
      filename[5] = (i%100)/10 + '0';
      filename[6] = i%10 + '0';
      if (!SD.exists(filename)) 
      {
        // only open a new file if it doesn't exist
        dataFile = SD.open(filename, FILE_WRITE); 
        break;
      }
    }
    
    //Serial.print("Logging to: ");
    Serial.println(filename);
    if (dataFile) {          */                          // if file can be opened, write to it
      //Serial.println("File opened for writing");
/*      dataFile.print("Date"); dataFile.print(", ");
      dataFile.print("Newtons"); dataFile.print(", ");
      dataFile.print("Celsius"); dataFile.print(", ");
      dataFile.print("X"); dataFile.print(", ");
      dataFile.print("Y"); dataFile.print(", "); 
      dataFile.println("Z");
      dataFile.close();             */
    /*}
    else {                                // if not, show an error
      Serial.println("SD error"); 
    }
  }*/
}

void loop() {
  /*float count = analogRead(A0);
  float voltage = count / 1023 * 5.0;// convert from count to raw voltage
  float force_reading = intercept + voltage * slope; //converts voltage to sensor reading

  prev_vals[val_num] = force_reading;
  val_num++;
  if (val_num >= NUM_HIST_VALS) {
    val_num = 0;
    zero_ready = true;
  }*/


  // --------------Grab Tempdata------------------------ 
  /*sensors.requestTemperatures();                        // Send the command to get temperatures
  tempc = sensors.getTempCByIndex(0);                   // Why "byIndex"? You can have more than one IC on the same bus. 
                                                        // 0 refers to the first IC on the wire
  SEND(temperature, tempc)*/
  
  /* ---Get a new sensor event */ 
  sensors_event_t event;
  mma.getEvent(&event);


  /*---------- Display the results (acceleration is measured in m/s^2) */ 
  
  x=event.acceleration.x;  y=event.acceleration.y;  z=event.acceleration.z;
  //storeData(force_reading, tempc, x, y, z);
  
  //SEND(force, force_reading)
  SEND(x, x)
  SEND(y, y)
  SEND(z, z)

  /*BEGIN_READ
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
  READ_DEFAULT(data_name, data) {
    Serial.print("Invalid: ");
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ*/
  
//  delay(10);
}
/*
void storeData(float force, float T, float x, float y, float z) {
   DateTime now = RTC.now();
   dataFile.print(now.year(), DEC);dataFile.print('/');
   dataFile.print(now.month(), DEC);dataFile.print('/');
   dataFile.print(now.day(), DEC);dataFile.print(' ');
   dataFile.print(now.hour(), DEC);dataFile.print(':');
   dataFile.print(now.minute(), DEC);dataFile.print('.');
   dataFile.print(now.second(), DEC);dataFile.print(", ");
    
   dataFile.print(force);dataFile.print(", ");
   dataFile.print(T);dataFile.print(", ");
   dataFile.print(x);dataFile.print(", "); 
   dataFile.print(y); dataFile.print(", ");dataFile.println(z);
} */
