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

     Clock Cycle     Button Reported
        ===========     ===============
        1               B -start?
        2               Y
        3               Select
        4               Start - down
        5               Up on joypad
        6               Down on joypad
        7               Left on joypad
        8               Right on joypad
        9               A
        10              X
        11              L
        12              R
        13              none (always high)
        14              none (always high)
        15              none (always high)
        16              none (always high)

   USER NOTES ::::::

   for the first four seconds after the Arduino boots 
   the board will be in a mode select state

  In order to select an operating mode both players must press one 
  of the following buttons as the same time

  A = players agree
  B = players alternate
  Y = players differ
  X = players both play
    
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
#define PLAYERS_AGREE 1
#define PLAYERS_ALTERNATE 2
#define PLAYERS_DIFFER 3
#define PLAYERS_BOTH_CONTROL 4
// number of players or controllers (might support more in future)
#define NUM_PLAYERS 2

// for dev and debug
bool DEBUG = true;
int mode = PLAYERS_AGREE;  
// dealing with which player is active as well
// as turn durations and lengths
int activePlayer = 0;
long turnStart;
long pastPoll;
long turnLength = 1500;
// the data that will be written to outputs

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
    startupMode();
    if (DEBUG) {
        Serial.begin(57600);
    }
}


void loop() {
    uint16_t output_state;
    turnLength = random(500,3000);
    switch (mode){
        case PLAYERS_ALTERNATE:
            output_state = playersAlternate(turnLength);
            break;
        case PLAYERS_AGREE:
            output_state = playersAgree();
            break;
        case PLAYERS_DIFFER:
            output_state = playersDiffer();
            break;
        case PLAYERS_BOTH_CONTROL:
            output_state = bothControl();
    }
    
    writeToChip(output_state);
    
    if (DEBUG) {
      delay(35);
    }
}


void startupMode() {
    // listen for 3 seconds for a button to have been pressed
    // on both controllers
    // store the last button that is pressed
    // each button corrisponds to an operating mode
    long then = millis();
    long selection_time = 4000;
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
        else if (combined_buttons & SNES_START) {
            DEBUG = true;  
        }
        else if (combined_buttons & SNES_SELECT) {
            DEBUG = false;
        }
    }
    
    mode = new_mode;
    
    if (DEBUG) {
        Serial.print("Mode : ");
        Serial.println(mode);
    }
}

// Function for sending the button data to
// The output controller
void writeToChip(uint16_t data) {
  PORTA = ~(byte)data;
  PORTC = ~((data >> 8) | 0xF0);
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
