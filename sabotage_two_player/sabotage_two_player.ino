/*
    Arduino Firmware for interfacing with the SNES

    This verson is specifically designed for fighting games
    where two players fight against each other as it allows

    Two Input Controllers : Two Output Controllers
  
    Programmed by Nathan Villicana-Shaw
    Fall 2015
    
    -----------------------------------------------------------
    This project is a work in progress and builds upon the shoulders
    of many brilliant programmers, hackers, engineers and artists
    
    ===========================================================
    ARDUINO : PORTS AND PINS  
    ===========================================================
    
    PLAYER 1 : OUTPUT
    -------------------------------------------------
    PORTA on the Arduino Mega consists of pins 22-29
      22 is bit 0, 29 is bit 7
      we use all the pins  (11111111)
       
    PORTC on the Arduino MEGA consists of pins 30-37
    `37 is bit 0, 30 is bit 7
      we use pins 34-37 (1111xxxx)
      
    PLAYER 2 : OUTPUT
    -------------------------------------------------
    PORTL : on the Arduino Mega consists of pins 42-49 
      49 is bit 0, 42 is bit 7 
      we use all the pins (11111111)
      
    PORTB : on the Arduino Mega consists of pins 10-13 and 50-53 
      53 is bit 0, 50 is bit 3, 10 is bit 4, 13 is bit 7 
      we use pins 50-53 (1111xxxx)

    ===================================================
    SNES CONTROLLERS
    ===================================================
    
    It is very important to note that the SNES controller reads a button as being pressed
    upon the grounding of a signal - Thus when a 0 is written to a Pin the button is "pressed"
    untill the pin is raised back to its HIGH
    
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

     Clock Cycle     Button Reported          MEGA PIN Player 1         MEGA PIN Player 1
     ===========     ===============          =================         =================
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

  =======================================================================
                        Mode Selection
  =======================================================================

  for the first four seconds after the Arduino boots 
  the board will be in a mode select state

  In order to select an operating mode both players must press one 
  of the following buttons as the same time

  A             = players agree
  B             = players alternate
  Y             = players differ
  X             = players both play
  Right Trigger = players alternate random
  Left Trigger  = unmapped
  D-Pad UP      = unmapped
  D-Pad DOWN    = unmapped
  D-Pad LEFT    = unmapped
  D-Pad Right   = two controls two
  SELECT        = Turns DEBUG off
  START         = Turns DEBUG on

  TODO ::

  Add modes for multiplayer games where you are fighting against someone.
    -  allow one player to hijack another players controller

  Rewrite the SNESpad library to include much of this functionality

  Flushout the player takeover system
    - perhaps the buttons become the other players D-pad?
  
  determineOutputController is broken fix it
  ==================================================================
*/
#include <SNESpad.h>
// the pins we use for receiving data from the two SNES controllers
#define TRIGGER_MASK 0x0C //bitmask for both of the triggers
// --------------------- Arduino Pins ------------------------
#define IN1_STROBE 2
#define IN1_CLOCK 3
#define IN1_DATA 4
#define IN2_STROBE 5
#define IN2_CLOCK 6
#define IN2_DATA 7
// ------------------------ MODES ----------------------------
// ultimate co-op functionality
#define SELECTION_MODE 0
#define PLAYERS_AGREE 1             // both players have to press a button
#define PLAYERS_ALTERNATE 2         // players take turns
#define PLAYERS_ALTERNATE_RANDOM 3  // players take turns of random length
#define PLAYERS_DIFFER 4            // if both players press a button it does not go through
#define PLAYERS_BOTH_CONTROL 5      //  both players signal gets passed w/o resistance
// modes of operation for 
// ultimate competative functionality
#define PLAYER_TAKEOVER 6           // you are able to "take over" opponent controls
// modes for playing solo
#define TWO_CONTROL_TWO 9           // each controller controls each output w/o resistance
#define TOGGLE_PLAYER 10            // you can manually switch between output controllers
// --------------------- Controlers and Players -------------------
// number of players or input controllers
#define NUM_PLAYERS 2

// for chip write modes
#define PLAYER_ONE 1
#define PLAYER_TWO 2

// for dev and debug
bool DEBUG = false;

// for determining operating mode
int mode = PLAYERS_AGREE;  

// dealing with which player is active as well
// as turn durations and lengths
int activePlayer = 0;
long turnStart;
long pastPoll;
long turnLength = 1500;
long selectPressTime = 3000;
long lastSelectPress;

long lastLeftTriggerPress;
long lastRightTriggerPress;
long triggerPressTime = 3000;

// variable to keep track of past button states
byte pastAState;
byte pastBState;
byte pastCState;
byte pastLState;

uint16_t pastOutputState = 0x0FFF;
uint16_t outputState;
uint16_t player1State;
uint16_t player2State;
//
SNESpad snes1 = SNESpad(IN1_STROBE, IN1_CLOCK, IN1_DATA);
SNESpad snes2 = SNESpad(IN2_STROBE, IN2_CLOCK, IN2_DATA);
    
void setup() {

    // set controller ports to outputs
    DDRA = 0xFF;
    DDRC = 0xF0;
    DDRL = 0xFF;
    DDRB = 0xF0;
    // set controller ports to HIGH
    PORTA = 0xFF;
    PORTC = 0xF0;
    PORTL = 0xFF;
    PORTB = 0xF0;
    
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
            writeToChip(output_state, PLAYER_ONE);
            break;
        case PLAYERS_ALTERNATE_RANDOM:
            output_state = playersAlternateRandom(turnLength);
            writeToChip(output_state, PLAYER_ONE);
            break;
        case PLAYERS_AGREE:
            output_state = playersAgree();
            writeToChip(output_state, PLAYER_ONE);
            break;
        case PLAYERS_DIFFER:
            output_state = playersDiffer();
            writeToChip(output_state, PLAYER_ONE);
            break;
        case PLAYERS_BOTH_CONTROL:
            output_state = bothControl();
            writeToChip(output_state, PLAYER_ONE);
            break;
        case PLAYER_TAKEOVER:
            playerTakeover();
            writeToChip(player1State, PLAYER_ONE);
            writeToChip(player2State, PLAYER_TWO);
            break;
        case SELECTION_MODE:
            mode = selectMode();
            break;
        case TWO_CONTROL_TWO:
            output_state = twoControlTwo();
            writeToChip(output_state, PLAYER_ONE);
            writeToChip(output_state, PLAYER_TWO);
            break;
        case TOGGLE_PLAYER:
            output_state = twoControlTwo();
            int outputController = determineOutputController();
            writeToChip(output_state, outputController);
            break;
        
    }
    
    checkSelect();
    pastOutputState = output_state;
    
    if (DEBUG) {
        delay(35);
    }
}

// ==============================================
// Helper Functions for Modes of Operations
// ==============================================
int determineOutputController(){
/*
   * This method allows a single player to toggle
   * their signal's output between the two output
   * controllers
   * 
   * this method only responds to the first input controller
   * as it assumes you are playing alone
   */
  // check to see if the trigger buttons state has changed
  if (~PORTC & TRIGGER_MASK && pastOutputState & TRIGGER_MASK){
    // if it is pressed and the last reading it was not pressed
    // set lastSelectPress to now
    lastSelectPress = millis();
    pastSelectValue = true;
  }
  // if they are off but were just turned on
  else if (!(~PORTC & TRIGGER_MASK) && !(pastOutputState & TRIGGER_MASK)) {
    // do something
  }
  //  
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

int selectMode() {
    // listen for 8 seconds for a button to have been pressed
    // on both controllers
    // store the last button that is pressed
    // each button corrisponds to an operating mode
    long then = millis();
    long selection_time = 8000;
    int new_mode = mode;
    /*
     * There is a better way to do this for sure
     */
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
            new_mode = mode;
        }
        else if (combined_buttons &  SNES_R) {
            new_mode = PLAYERS_ALTERNATE_RANDOM;
        }
        else if (combined_buttons & SNES_START) {
            DEBUG = true;  
        }
        else if (combined_buttons & SNES_SELECT) {
            DEBUG = false;
        }
        else if (combined_buttons & SNES_UP) {
            new_mode = mode;
        }
        else if (combined_buttons & SNES_DOWN) {
            new_mode = mode;
        }
        else if (combined_buttons & SNES_LEFT) {
            new_mode = TOGGLE_PLAYER;
        }
        else if (combined_buttons & SNES_RIGHT) {
            new_mode = TWO_CONTROL_TWO;
        }
    }
    
    if (DEBUG) {
        Serial.print("Mode : ");
        Serial.println(new_mode);
    }

    // this is to tell the players that the process is finished
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
void writeToChip(uint16_t data, int mode) {
  if (mode == PLAYER_ONE){
      PORTA = ~(byte)data;
      PORTC = ~((data >> 8) | 0xF0);
  }
  else if (mode == PLAYER_TWO){
      PORTL = ~(byte)data;
      PORTB = ~((data >> 8) | 0xF0);
  }
  checkSelect();
}


void checkSelect(){
  /*
   * This method enters into the selection mode if the
   * select button of the first players controller is
   * held down for selectPressTime 
   */
  // check to see if the select buttons state has changed
  if (~PORTA & SNES_SELECT && !(pastAState & SNES_SELECT)){
    // if it is pressed and the last reading it was not pressed
    // set lastSelectPress to now
    lastSelectPress = millis();
  }
  else if (~PORTA & SNES_SELECT && 
           pastOutputState & SNES_SELECT && 
           millis() > lastSelectPress + selectPressTime){
     PORTA = PORTA | 0x08;
     PORTA = PORTA & 0xF7;
     mode = SELECTION_MODE;
  }
  else{
  }
  
  if (DEBUG){
    Serial.print("Ports : ");
    printBits(data);
    Serial.println(" - ");
  }
}

// ------------ Modes of Operation -------------------
uint16_t playerTakeover() {
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  
  if (player1State & TRIGGER_MASK) {
      player2State = player1State;
  }

  if (player2State & TRIGGER_MASK) {
      player1State = player2State;
  }
  // future improvement :
  // if one player has both trigger held down their controls replace the opponent
  // for 100 ms, then their controls are mixed with their opponents controls for 900ms
}

uint16_t bothControl() {
  return playersAlternate(0);  
}

uint16_t twoControlTwo() {
    return snes1.buttons() | snes2.buttons();
}

uint16_t playersAgree() {
  /*
   * In this method the two input controllers are read
   * their button states are compared using boolean AND
   * and the results of the boolean operation are saved to state
   */
   uint16_t state;
   
   if (DEBUG){
      player1State = snes1.buttons();
      player2State = snes2.buttons();
      state = player1State & player2State;
      printBits(player1State);
      Serial.print("-");
      printBits(player2State);
      Serial.print("||");
      printBits(state);
      Serial.println(" ");
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
      player1State = snes1.buttons();
      player2State = snes2.buttons();
      state = player2State ^ player1State;
      printBits(player1State);
      Serial.print("-");
      printBits(player2State);
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
    Serial.print("Players Alernating  : ");
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
