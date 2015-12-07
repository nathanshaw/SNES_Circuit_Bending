/*
    Arduino Firmware for dealing with alternating between
    two SNES or NES controllers

    Programmed by Nathan Villicana-Shaw
    Fall 2015

    TODO :

    Add event polling system for sending the controller
    messages to the SNES

    Revamp code for MEGA    

    
    PORTA is pins 22 - 29 
    and will represent the second byte of the controller data
    PORTC is pins 30 - 37
    and will represent the first byte for the controllers

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
     5    Data2         ?
     6    IOBit         ?
     7    Ground        Brown
    
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
// number of players or controllers (might support more in future)
#define NUM_PLAYERS 2
// for dev and debug
#define DEBUG true

#define mode PLAYERS_ALTERNATE
// a few utility functions
#define setLow(port, pin) ((port) &= ~(1 << (pin)))
#define setHigh(port, pin) ((port) |= (1 << (pin)))
  
// dealing with which player is active as well
// as turn durations and lengths
int activePlayer = 0;
long turnStart;
long pastPoll;
int turnLength = 500;

// keep track of different controller states 
// could be cleaned up
uint16_t state1 = 0xFFFF;
uint16_t state2 = 0xFFFF;
uint16_t state = 0xFFFF;

// variable to keep track of past button states
uint16_t past_state = 0xFFFF;

SNESpad snes1 = SNESpad(IN1_STROBE, IN1_CLOCK, IN1_DATA);
// SNESpad snes2 = SNESpad(IN2_STROBE, IN2_CLOCK, IN2_DATA);
    
void setup() {
    if (DEBUG) {
        Serial.begin(57600);
    }
    // set controller ports to outputs
    DDRA = 0xFF;
    DDRC = 0xFF;
    // set controller ports to HIGH
    PORTA = 0xFF;
    PORTC = 0xFF;
}

void loop() {
    switch (mode){
        case PLAYERS_ALTERNATE:
            playersAlternate();
            break;
        case PLAYERS_AGREE:
            playersAgree();
            break;
    }
    writeToChip();
    delay(50);
}

void writeToChip() {
  // cycle through each bit in state and write it out to the correct pins
}

void playersAgree() {
  
  state1 = snes1.buttons();
  // state2 = snes2.buttons();
  state = snes1.buttons() & state2;

  if (DEBUG){
      printBits(state1);
      Serial.print("-");
      printBits(state2);
      Serial.print("||");
      printBits(state);
      Serial.println();
    }
}

// ------------ Modes of Operation -------------------

void playersAlternate() {
    // if the current turn is over
    // move to the next turn
    if (millis() > turnStart + turnLength){
        activePlayer = (activePlayer + 1) % NUM_PLAYERS;
        turnStart = millis();
        if (DEBUG){
          Serial.print("Player");
          Serial.println(activePlayer + 1);
        }
    }
  if (activePlayer == 1){
    state = state1 = snes1.buttons(); 
  }
  else if (activePlayer == 2) {
    // state = state2 = snes2.buttons(); 
  }
  if (DEBUG){
    printControllerStates(state1, state2);
  }
}

// ------------ Printing and DeBugging ----------------

void printControllerStates(uint16_t state1, uint16_t state2) {
  printBits(state1);
  Serial.print("-");
  printBits(state2);
  Serial.println(" ");
}
//overloaded to handle bytes and ints
void printBits(uint16_t myByte){
 for(uint16_t mask = 0x8000; mask; mask >>= 1){
   if(mask  & myByte)
       Serial.print('1');
   else
       Serial.print('0');
 }
}
