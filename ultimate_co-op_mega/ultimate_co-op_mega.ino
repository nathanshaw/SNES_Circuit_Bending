/*
    Arduino Firmware for dealing with alternating between
    two SNES or NES controllers

    Programmed by Nathan Villicana-Shaw
    Fall 2015
    
    PORTA on the Arduino Mega consists of pins 22 - 29 
    PORTC on the Arduino MEGA consists of pins 30 - 37
    
    When hooking up the SNES controllers there are a few things that help the process
     ----------------------------- ---------------------
    |                             |                      \
    | (1)     (2)     (3)     (4) |   (5)     (6)     (7) |
    |                             |                      /
     ----------------------------- ---------------------

    Pin   Description   Wire Color
    ===   ===========   ==========
     1    +5v (power)   White
     2    Clock         Yellow
     3    Strobe        Orange
     4    Data1         Red
     5    Nothing       ?
     6    Nothing       ?
     7    Ground        Brown

     Data Packing Order for the buttons. Note that the first clock cycle 
     is acutually the downwards edge of the STROBE

     Clock Cycle     Button Reported          Arduino MEGA pin
        ===========     ===============       =================
        1               B -start?                   22
        2               Y                           23
        3               Select                      24
        4               Start - down                25
        5               Up on joypad                26
        6               Down on joypad              27
        7               Left on joypad              28
        8               Right on joypad             29
        9               A                           37
        10              X                           36
        11              L                           35
        12              R                           34
        13              none (always high)         none
        14              none (always high)         none
        15              none (always high)         none
        16              none (always high)         none

   USER NOTES ::::::

  for the first four seconds after the Arduino boots 
  the board will be in a mode select state

  In order to select an operating mode both players must press one 
  of the following buttons as the same time

  A = players agree
  B = players alternate
  Y = players differ
  X = players both play

  TODO ::

  Add modes for multiplayer games weher you are fighting against someone.
    -  allow one player to hikack another players controller
    
*/

#include <SNESpad.h>
// the pins we use for receiving data from the two SNES controllers
#define IN1_STROBE 2
#define IN1_CLOCK 3
#define IN1_DATA 4
#define IN2_STROBE 5
#define IN2_CLOCK 6
#define IN2_DATA 7
// modes of operation
#define SELECTION_MODE 0
#define PLAYERS_AGREE 1
#define PLAYERS_ALTERNATE 2
#define PLAYERS_ALTERNATE_RANDOM 3
#define PLAYERS_DIFFER 4
#define PLAYERS_BOTH_CONTROL 5
// number of players or controllers (might support more in future)
#define NUM_PLAYERS 2

// for dev and debug
bool DEBUG = false;
int mode = PLAYERS_AGREE;  

// dealing with which player is active as well
// as turn durations and lengths
int activePlayer = 0;
long turnStart;
long pastPoll;
long turnLength = 1500;
// the data that will be written to outputs
long selectPressTime = 3000;
long lastSelectPress;
bool pastSelectValue;
// variable to keep track of past button states
uint16_t past_state = 0xFFFF;

SNESpad snes1 = SNESpad(IN1_STROBE, IN1_CLOCK, IN1_DATA);
SNESpad snes2 = SNESpad(IN2_STROBE, IN2_CLOCK, IN2_DATA);
    
void setup() {

    // set controller ports to outputs
    DDRA = 0xFF;
    DDRC = 0xFF;
    // set controller ports to HIGH
    PORTA = 0xFF;
    PORTC = 0xFF;
    // let things settle
    delay(10);
    mode = selectMode();
    if (DEBUG) {
        Serial.begin(57600);
    }
}


void loop() {
    uint16_t output_state;
    switch (mode){
        case PLAYERS_ALTERNATE:
            output_state = playersAlternate(turnLength);
            break;
        case PLAYERS_ALTERNATE_RANDOM:
            output_state = playersAlternateRandom(turnLength);
            break;
        case PLAYERS_AGREE:
            output_state = playersAgree();
            break;
        case PLAYERS_DIFFER:
            output_state = playersDiffer();
            break;
        case PLAYERS_BOTH_CONTROL:
            output_state = bothControl();
            break;
        case SELECTION_MODE:
            mode = selectMode();
            break;
    }
      
    writeToChip(output_state);
    
    if (DEBUG) {
        delay(35);
    }
}


int selectMode() {
    // listen for 3 seconds for a button to have been pressed
    // on both controllers
    // store the last button that is pressed
    // each button corrisponds to an operating mode
    long then = millis();
    long selection_time = 10000;
    int new_mode = mode;
    while (millis() < then + selection_time) {
        uint16_t combined_buttons = snes1.buttons() & snes2.buttons();
        if (combined_buttons & SNES_B){
            new_mode = PLAYERS_ALTERNATE;          
        }
        else if (combined_buttons & SNES_A){
            new_mode = PLAYERS_AGREE;
        }
        else if (combined_buttons & SNES_X){
            new_mode = PLAYERS_DIFFER;
        }
        else if (combined_buttons & SNES_Y){
            new_mode = PLAYERS_BOTH_CONTROL;
        }
        else if (combined_buttons & SNES_L){
            new_mode = PLAYERS_ALTERNATE_RANDOM;
        }
        else if (combined_buttons &  SNES_R) {
            new_mode = mode;
        }
        else if (combined_buttons & SNES_START) {
            DEBUG = true;  
        }
        else if (combined_buttons & SNES_SELECT) {
            DEBUG = false;
        }
    }
    
    if (DEBUG) {
        Serial.print("Mode : ");
        Serial.println(new_mode);
    }
    
    PORTA = PORTA | SNES_UP;
    delay(50);
    PORTA = PORTA & ~SNES_UP;
    PORTA = PORTA | SNES_DOWN;
    delay(50);
    PORTA = PORTA & ~SNES_DOWN;
    PORTA = PORTA | SNES_LEFT;
    delay(50);
    PORTA = PORTA & ~SNES_LEFT;
    PORTA = PORTA | SNES_RIGHT;
    delay(50);
    PORTA = PORTA & ~SNES_RIGHT;
    
    return new_mode;
}

// Function for sending the button data to
// The output controller
void writeToChip(uint16_t data) {
  PORTA = ~(byte)data;
  PORTC = ~((data >> 8) | 0xF0);
  // check to see if the select buttons state has changed
  if (~PORTA & 0x04 && pastSelectValue == false){
    // if it is pressed and the last reading it was not pressed
    // set lastSelectPress to now
    lastSelectPress = millis();
    pastSelectValue = true;
  }
  else if (~PORTA & 0x04 && pastSelectValue && millis() > lastSelectPress + selectPressTime){
     PORTA = PORTA | 0x08;
     PORTA = PORTA & 0xF7;
     mode = SELECTION_MODE;
  }
  else{
    pastSelectValue = PORTA & 0x04;
  }
  
  if (DEBUG){
    Serial.print("Ports : ");
    printBits(data);
    Serial.println(" - ");
  }
}

// ------------ Modes of Operation -------------------
uint16_t bothControl() {
  return playersAlternate(0);  
}

uint16_t playersAgree() {
  /*
   * In this method the two input controllers are read
   * their button states are compared using boolean AND
   * and the results of the boolean operation are saved to state
   */
   uint16_t state;
   
   if (DEBUG){
      uint16_t state1 = snes1.buttons();
      uint16_t state2 = snes2.buttons();
      state = state1 & state2;
      printBits(state1);
      Serial.print("-");
      printBits(state2);
      Serial.print("||");
      printBits(state);
      Serial.println();
   }
   else{
      state = snes1.buttons() & snes2.buttons();
   }
  
   return state;
}

uint16_t playersDiffer() {
  /*
   * This method reads both players controller
   * and performs an XOR boolean comparison between the
   * two controllers. The results are returned as an uint16_t
   */
   uint16_t state;
   if (DEBUG){
      uint16_t state1 = snes1.buttons();
      uint16_t state2 = snes2.buttons();
      state = state1 ^ state2;
      printBits(state1);
      Serial.print("-");
      printBits(state2);
      Serial.print("||");
      printBits(state);
      Serial.println(" ");
   }
   else {
      state = snes1.buttons() ^ snes2.buttons();
   }
   return state;
}

uint16_t playersAlternateRandom(long mTurnLength) {
  /*
   * This method checks to see who's "turn" it is
   * If it is a players turn their controller is read
   * and the results are outputed without any manipulation
   */
  uint16_t state;
   
  if (millis() > turnStart + mTurnLength){
        activePlayer = (activePlayer + 1) % NUM_PLAYERS;
        turnStart = millis();
        if (DEBUG){
            Serial.print("Player");
            Serial.println(activePlayer + 1);
        }
        turnLength = mTurnLength * random(200) * 0.01 + (mTurnLength * 0.1);
  
  }
  if (activePlayer == 0){
    state = snes1.buttons(); 
  }
  else if (activePlayer == 1) {
    state = snes2.buttons(); 
  }
  if (DEBUG){
    printBits(state);
  }
  return state;
}

uint16_t playersAlternate(long mTurnLength) {
  /*
   * This method checks to see who's "turn" it is
   * If it is a players turn their controller is read
   * and the results are outputed without any manipulation
   */
   uint16_t state;
  if (millis() > turnStart + mTurnLength){
        activePlayer = (activePlayer + 1) % NUM_PLAYERS;
        turnStart = millis();
        if (DEBUG){
            Serial.print("Player");
            Serial.println(activePlayer + 1);
        }
  }
  if (activePlayer == 0){
    state = snes1.buttons(); 
  }
  else if (activePlayer == 1) {
    state = snes2.buttons(); 
  }
  if (DEBUG){
    printBits(state);
  }
  return state;
}
 
// ------------ Printing and DeBugging ----------------

void printBits(uint16_t myByte){
 for(uint16_t mask = 0x8000; mask; mask >>= 1){
   if(mask  & myByte)
       Serial.print('1');
   else
       Serial.print('0');
 }
}
