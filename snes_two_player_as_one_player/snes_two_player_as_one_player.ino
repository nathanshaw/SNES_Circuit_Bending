/*
    Arduino Firmware for dealing with alternating between
    two SNES or NES controllers

    Programmed by Nathan Villicana-Shaw
    Fall 2015
*/
#include <SNESpad.h>
// the pins we use for receiving data from the two SNES controllers
#define IN1_STROBE 2
#define IN1_CLOCK 3
#define IN1_DATA 4
#define IN2_STROBE 5
#define IN2_CLOCK 6
#define IN2_DATA 7
// the pins we use for sending the data to a SNES.
#define OUT_DATA 8
#define OUT_STROBE 9
#define OUT_CLOCK 10
// modes of operation
#define PLAYERS_AGREE 1
#define PLAYERS_ALTERNATE 2
// number of players or controllers (might support more in future)
#define NUM_PLAYERS 2
// for dev and debug
#define DEBUG false

int mode = PLAYERS_ALTERNATE;

long pastPoll;
int turnLength = 500;

int activePlayer = 0;
long turnStart;

byte state1;
byte state2;
byte state;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(57600);
    // create object for each SNES controller
    SNESpad snes1 = SNESpad(IN1_STROBE, IN1_CLOCK, IN1_DATA);
    SNESpad snes2 = SNESpad(IN2_STROBE, IN2_CLOCK, IN2_DATA);
}


void loop() {
  // put your main code here, to run repeatedly:
  if (mode == PLAYERS_ALTERNATE) {
    playersAlternate();
  }
  else if (mode == PLAYERS_AGREE) {
    playersAgree();
  }
}


void playersAgree() {
  state1 = snes1.buttons();
  state2 = snes2.buttons();
  state = snes1.buttons() & state2;
  sendToSnes(agreed_state);
  if (DEBUG = true){
      printBits(state1);
      Serial.print("-");
      printBits(state2);
      Serial.print("||");
      printBits(agreed_state);
      Serial.println();
    }
}


void playersAlternate() {
    // if the current turn is over
    // move to the next turn
    if (millis() > turnStart + TURN_LENGTH){
        activePlayer = (activePlayer + 1) % NUM_PLAYERS;
        turnStart = millis();
    }
  
  if (activePlayer == 1){
    state = snes1.buttons(); 
  }
  else if (activePlayer == 2) {
    state = snes2.buttons(); 
  }
  if (DEBUG == true){
    printControllerStates(state1, state2);
  }
}

void printControllerStates(int state1, int state2) {
  printBits(state1);
  Serial.print("-");
  printBits(state2);
  Serial.println(" ");
}
// overloaded to account for bytes and ints
void printControllerStates(byte state1, byte state2) {
    printBits(state1);
    Serial.print("-");
    printBits(state2);
    Serial.println(" ");
}

// ------------ Printing and DeBugging ----------------

//overloaded to handle bytes and ints
void printBits(int myByte){
 for(int mask = 0x8000; mask; mask >>= 1){
   if(mask  & myByte)
       Serial.print('1');
   else
       Serial.print('0');
 }
}
 void printBits(byte myByte) {
 for(byte mask = 0x80; mask; mask >>= 1){
   if(mask  & myByte)
       Serial.print('1');
   else
       Serial.print('0');
 }
}
