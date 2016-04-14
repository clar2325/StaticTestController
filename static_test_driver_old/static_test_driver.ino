#include "communications.h"

// This is the information on the sensor being used. 
// See the www.vernier.com/products/sensors.

#define HIGH_RANGE

char Sensor[]="Force plate";
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

void setup() {
  Serial.begin(9600);
}

void loop() {
  float count = analogRead(A0);
  float voltage = count / 1023 * 5.0;// convert from count to raw voltage
  float reading = intercept + voltage * slope; //converts voltage to sensor reading

  prev_vals[val_num] = reading;
  val_num++;
  if (val_num >= NUM_HIST_VALS) {
    val_num = 0;
    zero_ready = true;
  }

  SEND(force, reading)

  BEGIN_READ
  READ_FLAG(zero) {
    if (zero_ready) {
      Serial.println("Zeroing");
      float zero_val = 0;
      for (int i = 0; i < NUM_HIST_VALS; i++)
        zero_val += prev_vals[i];
      zero_val /= NUM_HIST_VALS;
      intercept -= reading;
    }
    else {
      Serial.println("Zero not ready, please wait and try again");
    }
  }
  READ_DEFAULT(data_name, data) {
    Serial.print("Invalid command: ");
    Serial.print(data_name);
    Serial.print(":");
    Serial.println(data);
  }
  END_READ
  
//  delay(10);
}
