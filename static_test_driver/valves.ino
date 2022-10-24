class Valve
{
public:
  uint8_t m_valvepin;
  String m_valvename;
  String m_telemetry_id;

  Valve(uint8_t pin, String& name; String& telemetry) : m_valvepin{pin}, m_valvename{name}, m_telemetry_id{telemetry}{}

  void init_valve;
  void set_valve;
};

// valve initializer function
void Valve::init_valve(uint8_t pin) {
  pinMode(pin, OUTPUT);
}

// valve setting function
void Valve::set_valve(bool setting) {
  Serial.print(m_valvename);
  Serial.print(F(" to "));
  Serial.println(setting? F("open") : F("closed"));
  SEND_NAME(m_telemetry_id, setting);
  digitalWrite(m_valvepin, setting);
}

