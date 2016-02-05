/*
 *   Dirty POC program for operating the automated video switcher
 *   Sketch for controlling the AV switcher using an Arduino Nano and 8 relay switches
 */
// Arduino Pins (NANO)
uint8_t relayPins[] = {2, 3, 4, 5, 6, 7, 8, 11};
uint8_t rgbLedPins[] = {9, 10, 12};
int buttonPin = A0;
int potPin = A1;

// sensors
uint16_t potValue = 0;
bool buttonState = false;
bool lastButtonState = false;
uint64_t lastButtonPress = 0;

// switching output
uint64_t lastSwitch = 0;
float turnMult = 1.0;
uint16_t turnLength;
bool relayStates[] = {false, false, false, false,
                    false, false, false, false};
bool randomDst = false;

// debugging
uint8_t DEBUG = 1;

// Setup Loop

void setup() {
  // set relay pins as outputs
  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
  }
  // set led pins as outputs
  for (int i = 0; i < 3; i++) {
    pinMode(rgbLedPins[i], OUTPUT);
  }
  // set button pin as input
  pinMode(buttonPin, INPUT_PULLUP);
  //set pot pin as input
  pinMode(potPin, INPUT_PULLUP);
  
  if (DEBUG) {
    Serial.begin(57600);
  };
}

void loop() {
  // put your main code here, to run repeatedly:
  readSensors();
  switchOutput();
  if (DEBUG > 1) {delay(DEBUG);};
}


void switchOutput() {
  if (millis() > lastSwitch + turnLength) {
    if (randomDst) {
      int choosen = random(0, 8);
      for (int i = 0; i < 8; i++) {
        digitalWrite(relayPins[i], LOW);
        relayStates[i] = false;
      }
      digitalWrite(relayPins[choosen], HIGH);
      relayStates[choosen] = true;
    }
    else {
      for (int i = 0; i < 8; i++) {
        if (relayStates[i] == true) {
          digitalWrite(relayPins[i], LOW);
          relayStates[i] = false;
          digitalWrite(relayPins[(i + 1) % 7], HIGH);
          relayStates[(i + 1) % 7] = true;
        }
      }
    }
  }
}

void readSensors() {
  potValue = analogRead(potPin);
  lastButtonState = buttonState;
  randomDst = buttonState = digitalRead(buttonPin);
  turnLength = potValue * turnMult;
  if (lastButtonState == false) {
    if (buttonState == true) {
      lastButtonPress = millis();
    }
  }
}

