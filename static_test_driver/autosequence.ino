#define COUNTDOWN_DURATION 60000
//#define SAFETY_TIME -10000
#define START_TIME  0
#define RUN_TIME  30000
#define COOLDOWN_TIME 60000 * 5
#define ENGINE_COOLED 1
#define N 2

// TODO: I don't know if we need more states?  
// made some more states
enum state {
  MANUAL_CONTROL,
  START_UP,
  STAND_BY,
  TERMINAL_COUNT,
  IGNITION,
  ENGINE_RUNNING,
  COOL_DOWN,
  SHUT_DOWN
};

// Convinence
#define SET_STATE(STATE) {\
  state = STATE;          \
  SEND(status, #STATE);   \
}

long start_time = 0;
long shutdown_time = 0;
enum state state = START_UP;

//changed start_autosequence to start_countdown which is called in the standby state
void start_countdown() {
  Serial.println("Countdown started");
  SET_STATE(TERMINAL_COUNT)
  start_time = millis();
}

void abort_autosequence() {
  Serial.println("Autosequencer aborted");
  switch (state) {
    case START_UP:
    case STAND_BY:
    case TERMINAL_COUNT:
      SET_STATE(SHUT_DOWN)
      break;
    case IGNITION:

    case ENGINE_RUNNING:
      SET_STATE(COOL_DOWN)
      fuel_throttle(0);
      oxy_throttle(0);
      shutdown_time = millis();
      break;
    // etc, TODO
  }
}

void run_control() {
  long run_time = millis() - start_time - COUNTDOWN_DURATION;
  SEND(run_time, run_time)
  
  // TODO: There should be a lot more here, also checking if things went wrong
  switch (state) {
    case STAND_BY:
      //state that waits for a person to begin the test
    case START_UP:
      //state that checks all sensors and inital parameters
      for(int n=0; n<N; n++)
      {
        if(!temp_status)
        {
          Serial.println("Sensor Not Detected");
          n = 100; //this prevents the for function from continuing to loop
          abort_autosequence();
        }
        else if(n == 1) //gets to the end of the for loop
        {
          if(state == START_UP)
          {
            Serial.println("All Sensors Go");
            SET_STATE(STAND_BY)
          }
        }
      }
      break;
    case TERMINAL_COUNT:
      //countdown state
      if (run_time >= START_TIME) {
        SET_STATE(IGNITION)
        fuel_throttle(10);
        oxy_throttle(10);
        fire_ignitor();
      }
      break;
    case IGNITION://looked at past tests and put in some rough parameters for ignition. The temperature would rise by approx 8 degrees celsius in the span of 2-3 seconds
    //this case might be entirely unessesary but I added it just in case we might want it
      SET_STATE(ENGINE_RUNNING)
    case ENGINE_RUNNING:
      if (temp >= 60) //checking outlet temperature to see if it reaches a certain level
      {
        Serial.println("Temperature Reached Critical level");
        abort_autosequence();
      }
      if (run_time >= RUN_TIME) {
        SET_STATE(COOL_DOWN)
        fuel_throttle(0);
        oxy_throttle(0);
        shutdown_time = millis();
      }
      break;
    
    case COOL_DOWN:
      if (millis() - shutdown_time >= COOLDOWN_TIME) {
        Serial.println("Autosequencer finished");
        SET_STATE(SHUT_DOWN)
        start_time = 0;
      }
      break;
    case SHUT_DOWN:
      //state to end the program
      abort();
      break;
  }
  //update_throttle();
}

void run_manual_control() 
{
  // TODO: Read and update throttle settings
}


