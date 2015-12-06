#include <SNESpad.h>

// the pins we use for receiving data from the two SNES controllers
#define STROBE1 2
#define CLOCK1 3
#define DATA1 4
#define STROBE2 5
#define CLOCK2 6
#define DATA2 7
#define ledPin 13  // goes on when we're sending midi data

// the pins we use for sending the data to a SNES.
#define OUT_DATA 8
#define OUT_STROBE 9
#define OUT_CLOCK 10

// modes of operation
#define PLAYERS_AGREE 1
#define PLAYERS_ALTERNATE 2

// for dev and debug
#define DEBUG true

// TODO : create if in defines to add alternation time if players alternate


// create object for each SNES controller
SNESpad snes1 = SNESpad(STROBE1, CLOCK1, DATA1);
SNESpad snes2 = SNESpad(STROBE2, CLOCK2, DATA2);

int state1; // joystick button state
int state2;

int mode = PLAYERS_AGREE;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(38400);
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
  int agreed_state = state1 & state2;
  sendToSnes(agreed_state);
  printBits(state1);
  Serial.print("-");
  printBits(state2);
  Serial.print("||");
  printBits(agreed_state);
  Serial.println();
}

void playersAlternate() {
  state1 = snes1.buttons(); 
  sendToSnes(state1);
  state2 = snes2.buttons();
  sendToSnes(state2);
  if (DEBUG == true){
    printControllerStates(state1, state2);
  }
}

void printControllerStates(int state1, int state2) {
  printBits(state1);
  Serial.print("-");
  printBits(state2);
  delayMicroseconds(500);
  Serial.println();
}

void sendToSnes(int message) {
  // send the message out to the SNES
}

void printBits(int myByte){
 for(int mask = 0x8000; mask; mask >>= 1){
   if(mask  & myByte)
       Serial.print('1');
   else
       Serial.print('0');
 }
}
