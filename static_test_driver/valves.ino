class Valve
{
public:
  uint8_t m_valvepin;
  String m_valvename;
  String m_telemetry_id;
  bool m_current_state;

  Valve(uint8_t pin, String& name; String& telemetry) : m_valvepin{pin}, m_valvename{name}, m_telemetry_id{telemetry}, m_current_state{false} {}

  void init_valve();
  void set_valve();
};

// valve initializer function
void Valve::init_valve() {
  m_current_state = false;
  pinMode(m_valvepin, OUTPUT); 
  digitalWrite(m_valvepin, m_current_state);
}

// valve setting function
void Valve::set_valve(bool setting) {
  m_current_state = setting;
  Serial.print(m_valvename);
  Serial.print(F(" to "));
  Serial.println(m_current_state? F("open") : F("closed"));
  SEND_NAME(m_telemetry_id, m_current_state);
  digitalWrite(m_valvepin, m_current_state);
}

