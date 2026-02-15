// ESP32 + digital touch sensor (SIG -> GPIO13)
// Prints to Serial when touch state changes

const int TOUCH_PIN = 13;

// Debounce (helps avoid flicker / repeated triggers)
const unsigned long DEBOUNCE_MS = 30;

int lastStableState = -1;
int lastReadState = -1;
unsigned long lastChangeMs = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  // Many touch modules actively drive the output, but pulldown helps define LOW at boot.
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  lastReadState = digitalRead(TOUCH_PIN);
  lastStableState = lastReadState;

  Serial.println("Touch sensor ready on GPIO13.");
}

void loop() {
  int state = digitalRead(TOUCH_PIN);

  // If the reading changed, start (or restart) debounce timer
  if (state != lastReadState) {
    lastReadState = state;
    lastChangeMs = millis();
  }

  // If it has stayed the same long enough, accept it as the new stable state
  if ((millis() - lastChangeMs) > DEBOUNCE_MS && state != lastStableState) {
    lastStableState = state;

    if (lastStableState == HIGH) {
      Serial.println("TOUCHED");
    } else {
      Serial.println("RELEASED");
    }
  }

  delay(5);
}