#ifndef DEFINITIONS
#define DEFINITIONS

// SENSOR DEFINES

#define SENSOR_ERROR_LIMIT 5 // Max number of errors in a row before deciding a sensor is faulty

#define PRESSURE_CALIBRATION_FACTOR 246.58
#define PRESSURE_OFFSET 118.33
#define PRESSURE_MIN_VALID -100
#define PRESSURE_MAX_VALID 1000

#define TEMP_MIN_VALID -10
#define TEMP_MAX_VALID 120

#define FORCE_MIN_VALID -50
#define FORCE_MAX_VALID 500

#if CONFIGURATION == MK_2
#define LOAD_CELL_CALIBRATION_FACTOR 20400.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#else
#define LOAD_CELL_CALIBRATION_FACTOR 20400
#endif

#define LOAD_CELL_RETRY_INTERVAL 10
#define LOAD_CELL_MAX_RETRIES 20


// SENSOR DEFINES END 




//  IGNITER DEFINES

#define IGNITER_PIN 15

#if CONFIGURATION == DEMO
#define IGNITER_DURATION 5000
#else
#define IGNITER_DURATION 500
#endif


//IGNITER DEFINES END 





//LCD DEFINES BEGIN

#define WIDTH 16
#define SCROLL_PERIOD 700 // milliseconds

//LCD DEFINES END 





//STATIC TEST DRIVER DEFINES BEGIN 

#define INLET_TEMP A5
#define OUTLET_TEMP A4
#define NUMBER_OF_TEMP_SENSORS 4
int temp_error[NUMBER_OF_TEMP_SENSORS] = { 0,0 };

#define PRESSURE_FUEL A0
#define PRESSURE_OX A1
#define PRESSURE_FUEL_INJECTOR A2
#define PRESSURE_OX_INJECTOR A3
#define PRESSURE_NUM_HIST_VALS 10
#define NUMBER_OF_PRESSURE_SENSORS 4


//valves
#define FUEL_PRE_PIN 34
#define FUEL_MAIN_PIN 33
#define OX_PRE_PIN 32
#define OX_MAIN_PIN 31
#define N2_CHOKE_PIN 30
#define N2_DRAIN_PIN 29

// Load cell setup
#define LOAD_CELL_1_DOUT 11
#define LOAD_CELL_1_CLK 7
#define LOAD_CELL_2_DOUT 10
#define LOAD_CELL_2_CLK 6
#define LOAD_CELL_3_DOUT 9
#define LOAD_CELL_3_CLK 5
#define LOAD_CELL_4_DOUT 8
#define LOAD_CELL_4_CLK 4

//thermocouple 
#define THERMOCOUPLE_PIN_1 45 //PLACEHOLDER
#define THERMOCOUPLE_PIN_2 46 //PLACEHOLDER




//CLASS DECLARATIONS 

 
float mean(const float *data, unsigned int size) {
  float result = 0;
  for (int i = 0; i < size; i++)
      result += data[i];
  return result / size;
}


class LoadCell
{
  private:
  HX711 m_scale;

  public:
  uint8_t m_dout; //Digital out pin
  uint8_t m_clk;  //Clock pin
  double m_calibrationFactor;

  int m_error;

  float m_current_force;

  LoadCell(){}
  LoadCell(uint8_t dout, uint8_t clk) : m_calibrationFactor{LOAD_CELL_CALIBRATION_FACTOR} , m_dout {dout} , m_clk {clk}, m_error{0}, m_current_force{0} {}

  float read_force();
  void init_loadcell();

  void updateForces() {
    m_current_force = read_force();
  }

  void zeroForces(){
    m_scale.tare();
  }
};


class Thermocouple 
{
  private:
  Adafruit_MAX31855 m_thermocouple;
  public:
  int m_thermocouplepin;
  int m_error;
  const String m_sensor_name;
  const String m_sensor_short_name;

  float m_current_temp;

  Thermocouple(int8_t pin, const String& name, const String& shortname) : m_thermocouplepin {pin}, m_sensor_name { name } , m_sensor_short_name { shortname }, m_error{0} , m_thermocouple{pin}, m_current_temp{0} {} 
  
  void init_thermocouple();
  float read_thermocouple();
  float read_temp();

  void updateTemps() {
    m_current_temp = read_thermocouple();
  }

};


class PressureTransducer
{
  private:
  int m_current_hist_val;
  float m_pressure_history[PRESSURE_NUM_HIST_VALS];
  
  public:
  bool m_zero_ready;
  int m_pressurepin;
  int m_error;
  const String m_sensor_name;
  const String m_sensor_short_name;

  float m_tare;

  float m_current_pressure;
  
  PressureTransducer(int pin, const String& name, const String& shortname) : m_pressurepin{pin}, m_sensor_name{name}, m_sensor_short_name{shortname} , m_error{0}, m_tare{0}, m_pressure_history{}, m_current_hist_val{0}, m_zero_ready{false} {} 

  void init_transducer();
  float read_pressure();

  void updatePressures();

  void zeroPressures(){
      m_tare = mean(&m_pressure_history[0], PRESSURE_NUM_HIST_VALS);
  }
  
};


class Valve
{
public:
  uint8_t m_valvepin;
  String m_valvename;
  String m_telemetry_id;
  bool m_current_state;

  Valve(uint8_t pin, const String& name, const String& telemetry) : m_valvepin{pin}, m_valvename{name}, m_telemetry_id{telemetry}, m_current_state{false} {}

  void init_valve();
  void set_valve(bool setting);
};





#endif 
