/*  This is the Firmware for the SNES Arduino

     Coded by Nathan Villicana-Shaw
     In the Spring of 2015

     This Version of the SNES will have
        4 input jacks on the rear
        4 Digital Outputs
        4 PWM Outputs that pass through a simple highpass filter
        (in the circuit)
        2 Pots on the left for controlling the "fast" and "slow" speeds
        2 pots on the right for controlling the amplitude of the fast and slow outputs

      Digital Pin 2 will remain ground
      Digital Pin 4 will remain 5v
      // perhaps a switch should be added to flip this behavior
      Digital Pin 7 is the "slow" pin (mapped to A0 level)
      Digital Pin 8 is the "fast" pin (mapped to A1 level)

      PWM pin 3 ranges from 0 - 50%  (mapped to A0 level)
      PWM pin 5 ranges from 50% - 100% (mapped to A1 level)
      PWM pin 6 is the full range (mapped to A2 level)
      PWM pin 9 is the full range (mapped to A3 level)

      // this code is not optimized and exists in POF form at this itteration

      TODO :::
        -use faster hardware pwm instead of built in functions
        -add in the pots
        -add option for leds instead of pots
*/

#define INPUT_SCHEMA 0
#define ARDUINO_INPUTS 0

uint8_t DEBUG = 0;
// the pins we will be using
const int pwm_pins[] = {3, 5, 6, 9};
const int digital_pins[] = {2, 4, 7, 8};
const int input_pins[] = {A3, A2, A1, A0};
const int pot_pins[] = {A4, A5, A6, A7};
const int flipSwitch = 10;
// past for all groups
uint64_t digital_past[4];
uint64_t pwm_past[4];
uint64_t input_past[4];
uint64_t pot_past[4] = {100, 200, 300, 400};
uint64_t flip_past;
//
int digital_delay_low[4];
int digital_delay_high[4];
//
uint16_t pot_value[4];
float input_value[4];
uint16_t pwm_value[4];
float pwm_scaler[4];
bool digital_value[4];
bool flip_value;
bool pwm_toggle[4];
//
uint16_t INPUT_DELAY = 20;
uint16_t POT_DELAY = 20;

// for debugging
static int PRINT_INTERVAL = 60;
uint64_t last_print;

// =============================================================================
// -------------------------------- Setup Loop ---------------------------------
// =============================================================================

void setup() {
  Serial.begin(115200);
  for (int i; i < sizeof(pwm_pins) / 2; i++) {
    pinMode(pwm_pins[i], OUTPUT);
  }
  for (int i; i < sizeof(digital_pins) / 2; i++) {
    pinMode(digital_pins[i], OUTPUT);
  }
  for (int i; i < sizeof(input_pins) / 2; i++) {
    pinMode(input_pins[i], INPUT_PULLUP);
  }
  for (int i; i < sizeof(pot_pins) / 2; i++) {
    pinMode(pot_pins[i], INPUT);
  }
  // our digital value is set to the current position of the flip switch
  pinMode(flipSwitch, INPUT);
  flip_value = digitalRead(flipSwitch);
  // digital_pins[0] will be low
  // We use a digital pin instead of a lead directly to ground to
  // utalize the current limiting features provided by the pin
  digital_value[0] = false;
  digitalWrite(digital_pins[0], digital_value[0]);
  // digital_pins[1] will be high
  // We use a digital pin for high for the same reason as above
  // this method also allow us to flip these pins if we desire
  digital_value[1] = true;
  digitalWrite(digital_pins[1], digital_value[1]);
}

// =============================================================================
// --------------------------------- Main Loop ---------------------------------
// =============================================================================

void loop() {
  // read the pots
  //readSwitch();
  readPots();
  readInputs();
  updateOutputs();
  //printStats();
}

// =============================================================================
// --------------------------------- Functions ---------------------------------
// =============================================================================

void printStats() {
  if (DEBUG) {
    if (millis() > PRINT_INTERVAL + last_print) {
      Serial.print("INS: ");
      for (int i = 0; i < 4; i++) {
        if (input_value[i] < 100) {
          Serial.print(" ");

          if (input_value[i] < 10 ) {
            Serial.print(" ");
          }
        }
        Serial.print(input_value[i]);
        Serial.print(" ");
      }

      Serial.print("| ");
      Serial.print("POTS:");
      //print pots
      for (int i = 0; i < 4; i++) {
        if (pot_value[i] < 1000) {
          Serial.print(" ");
          if (pot_value[i] < 100) {
            Serial.print(" ");
            if (pot_value[i] < 10) {
              Serial.print(" ");
            }
          }
        }
        Serial.print(pot_value[i]);
        Serial.print(" ");
      }
      Serial.print("| ");
      Serial.print("D-OUTS : ");
      for (int i = 0; i < 4; i++) {
        Serial.print(digital_value[i]);
        Serial.print(" ");
      }
      Serial.print("| ");
      Serial.print("PWM_O:");
      for (int i = 0; i < 4; i++) {
        if (pwm_value[i] < 100) {
          Serial.print(" ");
          if (pwm_value[i] < 10) {
            Serial.print(" ");
          }
        }

        Serial.print(pwm_value[i]);
        Serial.print(" ");
      }
      Serial.print("| ");
      Serial.print("SCALERS:");
      for (int i = 0; i < 4; i++) {
        Serial.print(digital_delay_high[i]);
        Serial.print("-");
        Serial.print(digital_delay_low[i]);
        Serial.print(" ");
      }
      Serial.println();
      last_print = millis();
    }
  }
}

void updateOutputs() {
  for (int i = 2; i < 4; i++) {
    if (digital_value[i] == false) {
      if (millis() > digital_past[i] + digital_delay_low[i]) {
        digital_value[i] = !digital_value[i];
        digitalWrite(digital_pins[i], digital_value[i]);
        digital_past[i] = millis();
      }
    }
    else {
      if (millis() > digital_past[i] + digital_delay_high[i]) {
        digital_value[i] = !digital_value[i];
        digitalWrite(digital_pins[i], digital_value[i]);
        digital_past[i] = millis();
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    if (pwm_toggle[i] == true) {
      if (millis() > pwm_past[i] + digital_delay_low[i]) {
        static float temp;
        temp = (pwm_scaler[i] * 125);
        pwm_value[i] = (byte) temp;
        analogWrite(pwm_pins[i], pwm_value[i]);
        pwm_toggle[i] = !pwm_toggle[i];
        pwm_past[i] = millis();
      }
    }
    else {
      if (millis() > pwm_past[i] + digital_delay_high[i]) {
        analogWrite(pwm_pins[i], LOW);
        pwm_value[i] = 0;
        pwm_toggle[i] = !pwm_toggle[i];
        pwm_past[i] = millis();
      }
    }
  }
}

void readSwitch() {
  // read the flipSwitch
  if (millis() > INPUT_DELAY * 2 + flip_past) {
    // if the flip switch has been flipped
    // we reverse the polarity for our first two digital Pins
    if (analogRead(flipSwitch) != flip_value) {
      digital_value[0] != digital_value[0];
      digital_value[1] != digital_value[1];
      digitalWrite(digital_pins[0], digital_value[0]);
      digitalWrite(digital_pins[1], digital_value[1]);
      flip_past = millis();
    }
  }
}

void readPots() {
  for (int i = 0; i < 4; i++) {
    if (millis() > POT_DELAY + pot_past[i]) {
      pot_value[i] = analogRead(pot_pins[i]);
      pot_past[i] = millis();
      switch (i) {
        case 0:
          digital_delay_low[0] = pot_value[0] * 2 * input_value[0];
          digital_delay_low[2] = pot_value[0] * 4 * input_value[2];
          digital_delay_high[0] = pot_value[0] * 2 * (1 - input_value[0]);
          digital_delay_high[2] = pot_value[0] * 4 * (1 - input_value[2]);
          break;
        case 1:
          digital_delay_low[1] = pot_value[1] * 3 * input_value[1];
          digital_delay_low[3] = pot_value[1] * 5 * input_value[3];
          digital_delay_high[1] = pot_value[1] * 3 * (1 - input_value[i]);
          digital_delay_high[3] = pot_value[1] * 5 * (1 - input_value[3]);
          break;
        case 2:
          pwm_scaler[0] = pwm_scaler[1] = (float)pot_value[3] * 0.000975;
          break;
        case 3:
          pwm_scaler[2] = pwm_scaler[3] = (float)pot_value[2] * 0.000975;
          break;
      }
    }
  }
}

void readInputs() {
  // read the external inputs from the outside
  // these are the pins that are wired to the jacks on the rear of the SNES's
  // these should be wired from A0-A3
  for (int i = 0; i < 4; i++) {
    // store reading into a temp variable
    static int input_temp;
    static float temp_map;
    input_temp = analogRead(input_pins[i]);
    temp_map = map(input_temp, 560, 775, 0, 1023);
    if(i % 2 == 0){
    input_value[i] = temp_map / 1020;
    }
    else{
      input_value[i] = 1 - (temp_map / 1020);
    }
  }
}



