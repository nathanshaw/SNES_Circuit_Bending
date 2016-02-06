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
float turnMult = 10.0;
uint16_t turnLength;
bool relayStates[] = {true, false, false, false,
                    false, false, false, false};

bool randomDst = true;

// debugging
uint8_t DEBUG = 0;

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
  pinMode(buttonPin, INPUT);
  //set pot pin as input
  pinMode(potPin, INPUT);
  
  if (DEBUG) {
    Serial.begin(57600);
  };
}

void loop() {
  // put your main code here, to run repeatedly:
  readSensors();
  switchOutput();
  if (DEBUG > 1) {printStats(DEBUG);};
}

void printStats(uint8_t debug) {
  delay(debug);
  Serial.print("Relay States : ");
  for (int i = 0; i< 8; i++) {
    Serial.print(relayStates[i]);
  }
  Serial.print(" pot : ");
  Serial.print(potValue);
  Serial.print(" button state : ");
  Serial.print(buttonState);
  Serial.print(" random dst : ");
  Serial.println(randomDst);
}

void switchOutput() {
   if (millis() > lastSwitch + turnLength) {
    lastSwitch = millis();
    if (randomDst) {
      if(DEBUG){Serial.print(" random ");};
      int choosen = random(0, 8);
      for (int i = 0; i < 8; i++) {
        digitalWrite(relayPins[i], LOW);
        relayStates[i] = false;
      }
      digitalWrite(relayPins[choosen], HIGH);
      relayStates[choosen] = true;
    }
    else {
      if(DEBUG){Serial.print(" ordered ");};
      for (int i = 0; i < 8; i++) {
        if (relayStates[i] == true) {
          if(DEBUG){Serial.print(" entered if ");};
          digitalWrite(relayPins[i], LOW);
          relayStates[i] = false;
          digitalWrite(relayPins[(i + 1) % 7], HIGH);
          relayStates[(i + 1) % 7] = true;
          break;
        }
      }
    }
  }
}

void readSensors() {
  potValue = analogRead(potPin);
  lastButtonState = buttonState;
  buttonState = digitalRead(buttonPin);
  if(buttonState == lastButtonState) {
    randomDst = buttonState;
  }
  turnLength = potValue * turnMult;
  if (lastButtonState == false) {
    if (buttonState == true) {
      lastButtonPress = millis();
    }
  }
}

