/*  This is the Firmware for the SNES Arduino
 *   
 *   Coded by Nathan Villicana-Shaw
 *   In the Spring of 2015
 *   
 *   This Version of the SNES will have 
 *      4 input jacks on the rear
 *      4 Digital Outputs
 *      4 PWM Outputs that pass through a simple highpass filter 
 *      (in the circuit)
 *   
 *    Digital Pin 2 will remain ground 
 *    Digital Pin 4 will remain 5v
 *    // perhaps a switch should be added to flip this behavior
 *    Digital Pin 7 is the "slow" pin (mapped to A0 level)
 *    Digital Pin 8 is the "fast" pin (mapped to A1 level)
 * 
 *    PWM pin 3 ranges from 0 - 50%  (mapped to A0 level)
 *    PWM pin 5 ranges from 50% - 100% (mapped to A1 level)
 *    PWM pin 6 is the full range (mapped to A2 level)
 *    PWM pin 9 is the full range (mapped to A3 level)
 *    
 *    // this code is not optimized and exists in POF form at this itteration
 */

// the pins we will be using
const int pwm_pins[] = {3,5,6,9};
const int digital_pins[] = {2, 4, 7, 8};
const int input_pins[] = {A0, A1, A2, A3};
const int flipSwitch = 10;
// past for all groups
long int digital_past[4];
long int pwm_past[4];
long int input_past[4];
long int flip_past;
//
int digital_delay[4];
// 
int input_value[4];
bool digital_value[4];
bool flip_value;
//
int SENSOR_DELAY = 20;

// for debugging
static int PRINT_INTERVAL = 500;
long int last_print;

void setup(){
    Serial.begin(9600);
    for (int i; i < sizeof(pwm_pins)/2; i++){
        pinMode(pwm_pins[i], OUTPUT);
    }
    for (int i; i < sizeof(digital_pins)/2; i++){
        pinMode(digital_pins[i], OUTPUT);
    }
    for (int i; i < sizeof(input_pins)/2; i++){
        pinMode(input_pins[i], INPUT);
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

void loop(){
    // read the pots
    readInputs();
    updateOutputs();
    printStats();
}

void printStats(){
    if (millis() > PRINT_INTERVAL + last_print){
      Serial.println("- - - - - - - - - - - - - - -");
      Serial.print("Current Digital Values  : ");
      for (int i; i < sizeof(digital_value)/2; i++){
        Serial.print(digital_value[i]); 
        Serial.print(" ");     
      }
      Serial.println();
      Serial.print("Current Analoge Values  : ");
      for (int i; i < sizeof(digital_value)/2; i++){
        Serial.print(digital_value[i]); 
        Serial.print(" ");     
      }
      Serial.println();
    }
}

void updateOutputs(){
  for (int i; i < sizeof(digital_pins)/2; i++){
          if(millis() > digital_past[i] + digital_delay[i] && i > 1){
            digital_value[i] != digital_value[i];
            digitalWrite(digital_pins[i], digital_value[i]);
            digital_past[i] = millis();
          }
  }
}

void readInputs(){
  
    // read the flipSwitch
    if (millis() > SENSOR_DELAY*2 + flip_past){
      // if the flip switch has been flipped
      // we reverse the polarity for our first two digital Pins
      if (analogRead(flipSwitch) != flip_value){
        digital_value[0] != digital_value[0];
        digital_value[1] != digital_value[1];
        digitalWrite(digital_pins[0], digital_value[0]);
        digitalWrite(digital_pins[1], digital_value[1]);
        flip_past = millis();
      }
    }
    
    // read the external inputs from the outside
    // these are the pins that are wired to the jacks on the rear of the SNES's
    // these should be wired from A0-A3
    
    for (int i; i < sizeof(input_pins)/2; i++){
        if (millis() > SENSOR_DELAY + input_past[i]){
            // store reading into a temp variable
            int temp = analogRead(input_pins[i]);
            // if the reading is different than the value we have stored
            if (temp != input_value[i]){
              // update our stored value
              input_value[i] = temp;
              // depending on what input we are reading then update accordingly
              switch (i){
             
              case 0: // first input
                // 1/2 power pwm pin
                analogWrite(pwm_pins[i], input_value[i]*0.125);
                break;
              case 1: // second input
                analogWrite(pwm_pins[i], input_value[i]*0.125 + 125);
                break;
              default: // last two inputs
                analogWrite(pwm_pins[i], input_value[i]*0.25);
                // this changes the delay for the slow and fast digital pins
                // it ranges from 20ms to about two seconds
                digital_delay[i] = (i * input_value[i] * 0.4) + 20;
                break;
              }
            }
            input_past[i] = millis();
        } 
    }
}


