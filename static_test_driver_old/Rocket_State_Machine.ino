#define START_UP 0
#define STAND_BY 1
#define COUNT_DOWN 2
#define IGNITION 3
#define COOL_DOWN 4
#define SHUT_DOWN 5

enum state {
  START_UP
  STAND_BY
  COUNT_DOWN
  IGNITION
  COOL_DOWN
  SHUT_DOWN
};

#define SET_STATE(STATE){
  state = STATE;
  SEND(status, #STATE);
}




void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  switch(state)
  {
    case START_UP:
      ready = check_pressure(); //check pressure function needs to output serial lines to describe problem
      if(ready)
      {
        state = STAND_BY;
      }
      else
      {
        state = SHUT_DOWN;
      }


    case STAND_BY:
      //lights up an LED to show standing by
      cdstart = manual_input(); //press button to start
      if(cdstart)
      {
        state = COUNT_DOWN;
      }


    case COUNT_DOWN:
      //Display timed countdown
      start_data_record();
      if(cdtime <= 10)
      {
        fuel_saftey(ON);
        oxy_saftey(ON);  
      }
      if(cdtime <= 5)
      {
        fuel_throttle(UP);
        oxy_throttle(UP);
      }
      if(cdtime <= 0)
      {
        state = IGNITION;
      }


    case IGNITION:
      ignite_engine();
      wait("insert time to wait")
      ignited = temp_check();
      if(!ignited)
      {
        state = SHUT_DOWN;
      }
      while(ignited)
      {
        burnout = burnout_check();//check based on temperature and force output
        if(burnout)
        {
          state = COOL_DOWN
        }
      }

    case COOL_DOWN:
      fuel_saftey(OFF);
      oxy_saftey(OFF);
      fuel_throttle(DOWN);
      oxy_throttle(DOWN);
      safe = cool_down_timer();
      if(safe == 0)
      {
        state = SHUT_DOWN;
      }

    case SHUT_DOWN:
      fuel_saftey(OFF);
      oxy_saftey(OFF);
      fuel_throttle(DOWN);
      oxy_throttle(DOWN);
      
  }
}
