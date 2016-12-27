int temp_0_pin = A0, temp_1_pin = A1, start_input_pin = A2, manual_abort_pin = A3;

void setup()
{
  Serial.begin(230400);
  Serial.println("Initializing...");
  pinMode(temp_0_pin, INPUT);
  pinMode(temp_1_pin, INPUT);
  pinMode(start_input_pin, INPUT);
  pinMode(manual_abort_pin, INPUT);
  /*
  //------------- set up temp sensor-----------
  sensors.begin();

  for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
    oneWire.search(addr[i]); // address on 1wire bus
  
    oneWire.reset();         // reset 1-Wire
    oneWire.select(addr[i]); // select DS18B20
  
    oneWire.write(0x4E);     // write on scratchPad
    oneWire.write(0x00);     // User byte 0 - Unused
    oneWire.write(0x00);     // User byte 1 - Unused
    oneWire.write(0x1F);     // set up 9 bits (0x1F)
    
    oneWire.reset();         // reset 1-Wire

    oneWire.write(0x48);     // copy scratchpad to EEPROM
    delay(15);               // wait for end of write
  }
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  */
}

void loop()
{
  run_control();
  float t_0 = analogRead(temp_0_pin);
  float t_1 = analogRead(temp_1_pin);
  if(digitalRead(start_input_pin) == HIGH)
  {
    start_input = 1;
  }
  if(digitalRead(manual_abort_pin) == HIGH && state != COOL_DOWN && state != SHUT_DOWN)
  {
    errormsg = "Manual Shutdown";
    abort_autosequence();
  }
  temp[0] = (t_0-8)/6.4; //this makes the range of outputs approx 0-100
  temp[1] = (t_1-8)/6.4;
}


