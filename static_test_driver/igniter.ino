#if CONFIGURATION == DEMO
#define IGNITER_DURATION 5000
#else
#define IGNITER_DURATION 500
#endif


long ignition_time = 0;
bool ignitor_active = false;

void init_igniter(int pin){
  pinMode(pin, OUTPUT);
}

void fire_igniter() {
  Serial.println("Firing igniter");
  digitalWrite(IGNITER_PIN, 1);
  ignition_time = millis();
  ignitor_active = true;
}

void reset_igniter() {
  Serial.println("Reset igniter");
  digitalWrite(IGNITER_PIN, 0);
  ignitor_active = false;
}

void handle_igniter() {
  if (ignitor_active && millis() > ignition_time + IGNITER_DURATION) {
    reset_igniter();
  }
}

