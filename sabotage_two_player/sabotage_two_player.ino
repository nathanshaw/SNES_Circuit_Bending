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
      
    --------------------------------------------------
    PLAYER 2 : OUTPUT
    -------------------------------------------------
    PORTL : on the Arduino Mega consists of pins 42-49
      49 is bit 0, 42 is bit 7
      we use all the pins (11111111)

    PORTK : on the Arduino Mega consists of pins 10-13 and 50-53
      53 is bit 0, 50 is bit 3, 10 is bit 4, 13 is bit 7
      we use pins 50-53 (1111xxxx)

    --------------------------------------------------
    LEDS : 
    --------------------------------------------------
    PORTF : on the Arduno Mega port F consists of analog
      pins 0 - 7, each LED corrisponds with an operation mode 

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
        1               B -start?                   22                          49
        2               Y                           23                          48
        3               Select                      24                          47
        4               Start - down                25                          46
        5               Up on joypad                26                          45  
        6               Down on joypad              27                          44
        7               Left on joypad              28                          43
        8               Right on joypad             29                          42
        9               A                           37                          53
        10              X                           36                          52
        11              L                           35                          51
        12              R                           34                          50
        13              none (always high)         none                        none
        14              none (always high)         none                        none
        15              none (always high)         none                        none 
        16              none (always high)         none                        none

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
  Right Trigger = debug on
  Left Trigger  = debug off
  D-Pad UP      = players alternating random
  D-Pad DOWN    = unmapped
  D-Pad LEFT    = toggler player // needs testing
  D-Pad Right   = two controls two // needs testing
  SELECT        = unmapped
  START         = Turns DEBUG on

  TODO ::

  Add modes for multiplayer games where you are fighting against someone.
    -  allow one player to hijack another players controller

  Rewrite the SNESpad library to include much of this functionality

  Flushout the player takeover system
    - perhaps the buttons become the other players D-pad?

  determineOutputController is broken fix it

  make leds blink durring mode selection mode
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
#define PLAYERS_AGREE 0x01             // both players have to press a button
#define PLAYERS_ALTERNATE 0x02         // players take turns
#define PLAYERS_ALTERNATE_RANDOM 0x04  // players take turns of random length
#define PLAYERS_DIFFER 0x08            // if both players press a button it does not go through
#define PLAYERS_BOTH_CONTROL 0x10      //  both players signal gets passed w/o resistance
#define PLAYERS_DIVERGE 0x20
// modes of operation for
// ultimate competative functionality
#define PLAYER_TAKEOVER 7           // you are able to "take over" opponent controls
// modes for playing solo
#define TWO_CONTROL_TWO 9           // each controller controls each output w/o resistance
#define TOGGLE_PLAYER 10            // you can manually switch between output controllers
// --------------------- Controlers and Players -------------------
// number of players or input controllers
#define NUM_PLAYERS 2

// for chip write modes
#define PLAYER_ONE 1
#define PLAYER_TWO 2
// ---------------------- Globals ---------------------------------
// for dev and debug
bool DEBUG = true;

// for determining operating mode
int mode = SELECTION_MODE;

// dealing with which player is active as well
// as turn durations and lengths
int activePlayer = PLAYER_ONE;
long turnStart;
long pastPoll;
long turnLength = 2000;
long divergeTurnLength = 1;
int divergeDirection;
long selectPressTime = 1500;
long lastSelectPress;

long lastLeftTriggerPress;
long lastRightTriggerPress;
long triggerPressTime = 2000;
long lastLedBlink;

// variable to keep track of past button states
uint16_t lastOutputState = 0x0FFF;
uint16_t outputState = 0x0000;
uint16_t player1State = 0x0000;
uint16_t player2State = 0x0000;
//
SNESpad snes1 = SNESpad(IN1_STROBE, IN1_CLOCK, IN1_DATA);
SNESpad snes2 = SNESpad(IN2_STROBE, IN2_CLOCK, IN2_DATA);

// ------------------------------------------------------------------
void setup() {

  // set controller ports to outputs
  DDRA = 0xFF;
  DDRC = 0xFF;
  DDRL = 0xFF;
  DDRK = 0xFF;
  // set LED's port to output
  DDRF = 0xFF;
  // set controller ports to HIGH (the resting state)
  PORTA = 0xFF;
  PORTC = 0xFF;
  PORTL = 0xFF;
  PORTK = 0xFF;
  // set LED's port to LOW
  PORTF = 0x00;
  
  // let things settle
  delay(10);
  mode = selectMode();
  
  if (DEBUG) {
    Serial.begin(57600);
    Serial.println("SERIAL BUS OPENED");
  }
}


void loop() {
  switch (mode) {
    // -------------------------------------------------
    // Modes for two players playing as one character
    // -------------------------------------------------
    case PLAYERS_AGREE:
      outputState = playersAgree();
      writeToChip(outputState, PLAYER_ONE);
      break;
    case PLAYERS_ALTERNATE:
      outputState = playersAlternate(turnLength);
      writeToChip(outputState, PLAYER_ONE);
      break;
    case PLAYERS_ALTERNATE_RANDOM:
      outputState = playersAlternateRandom(turnLength);
      writeToChip(outputState, PLAYER_ONE);
      break;
    case PLAYERS_DIFFER:
      outputState = playersDiffer();
      writeToChip(outputState, PLAYER_ONE);
      break;
    case PLAYERS_BOTH_CONTROL:
      outputState = bothControl();
      writeToChip(outputState, PLAYER_ONE);
      break;
    case PLAYERS_DIVERGE:
      divergeTurnLength = playersDiverge();
      playersAlternate(divergeTurnLength);
      writeToChip(outputState, PLAYER_ONE);
      break;
    // same as players bot control but output goes to both players
    case TWO_CONTROL_TWO:
      outputState = bothControl();
      writeToChip(outputState, PLAYER_ONE);
      writeToChip(outputState, PLAYER_TWO);
      break;
    // -------------------------------------------------
    // Modes for one player play as multiple characters
    // -------------------------------------------------
    case TOGGLE_PLAYER:
      outputState = bothControl();
      activePlayer = determineOutputController();
      writeToChip(outputState, activePlayer);
      break;
    //--------------------------------------------------
    // Modes for two players playing a VS game (KillerInstinct)
    // -------------------------------------------------  
    case PLAYER_TAKEOVER:
      playerTakeover();
      writeToChip(player1State, PLAYER_ONE);
      writeToChip(player2State, PLAYER_TWO);
      break;
    // -------------------------------------------------
    // The Selection Mode for switching between all the other modes
    // -------------------------------------------------  
    case SELECTION_MODE:
      mode = selectMode();
      break;
    // -----------------------
  }
  // turn on LED's for mode
  PORTF = mode;
  checkSelect();
  lastOutputState = outputState;

  if (DEBUG) {
    Serial.print(mode);
    Serial.print(" : ");
    // delay(10);
  }
}

// ==========================================================================
// Helper Functions for Modes of Operations
// ==========================================================================
int determineOutputController() {
  /*
       This method allows a single player to toggle
       their signal's output between the two output
       controllers

       this method only responds to the first input controller
       as it assumes you are playing alone
  */
  // check to see if the trigger buttons state has changed
  if (~PORTC & TRIGGER_MASK && lastOutputState & TRIGGER_MASK) {
    // if it is pressed and the last reading it was not pressed
    // set lastSelectPress to now
    lastSelectPress = millis();
  }
  // if they are off but were just turned on
  else if (!(~PORTC & TRIGGER_MASK) && !(lastOutputState & TRIGGER_MASK)) {
    // do something
  }
  // if the trigger buttons are held down for select press time flip the activePlayer
  else if (~PORTC & TRIGGER_MASK &&
           lastOutputState & TRIGGER_MASK &&
           millis() > lastSelectPress + selectPressTime) {
    if (activePlayer = PLAYER_ONE) {
      return PLAYER_TWO;
    }
    else {
      return PLAYER_ONE;
    }
  }
  return activePlayer;
}

int selectMode() {
  /*
  // listen for 5 seconds for a button to have been pressed
  // on both controllers
  // store the last button that is pressed
  // each button corrisponds to an operating mode
  */
  int new_mode = mode;
  if (DEBUG) {Serial.println("ENTERED MODE SELECT :");}
  /*
     There is a better way to do this for sure
  */
    uint16_t combined_buttons = snes1.buttons() & snes2.buttons();

    if (combined_buttons & SNES_Y) {
      new_mode = PLAYERS_ALTERNATE;
      if (DEBUG) {Serial.println("PLAYERS ALTERNATE SELECTED");}
    }
    else if (combined_buttons & SNES_A) {
      new_mode = PLAYERS_AGREE;
      if (DEBUG) {Serial.println("PLAYERS AGREE SELECTED");}
    }
    else if (combined_buttons & SNES_X) {
      new_mode = PLAYERS_DIFFER;
      if (DEBUG) {Serial.println("PLAYERS DIFFER SELECTED");}
    }
    else if (combined_buttons & SNES_B) {
      new_mode = PLAYERS_BOTH_CONTROL;
      if (DEBUG) {Serial.println("PLAYERS BOTH CONTROL SELECTED");}
    }
    else if (combined_buttons &  SNES_UP) {
      new_mode = PLAYERS_ALTERNATE_RANDOM;
      if (DEBUG) {Serial.println("PLAYERS ALTERNATE RANDOM SELECTED");}
    }
    else if (combined_buttons & SNES_R) {
      DEBUG = true;
      if (DEBUG) {Serial.println("DEBUG TURNED ON");}
    }
    else if (combined_buttons & SNES_L) {
      if (DEBUG) {Serial.println("DEBUG TURNED OFF");}
      DEBUG = false;
    }
    else if (combined_buttons & SNES_DOWN) {
      new_mode = PLAYERS_DIVERGE;
      if (DEBUG) {Serial.println("PLAYERS DIVERGE");}
    }
    else if (combined_buttons & SNES_LEFT) {
      new_mode = TOGGLE_PLAYER;
      if (DEBUG) {Serial.println("TOGGLE PLAYER SELECTED");}
    }
    else if (combined_buttons & SNES_RIGHT) {
      new_mode = TWO_CONTROL_TWO;
      if (DEBUG) {Serial.println("TWO CONTROL TWO SELECTED");}
    }

  if (DEBUG) {
    Serial.print("NEW MODE IS : ");
    Serial.println(new_mode);
  }

  return new_mode;
}

void moveJoystick(int delayTime) {
    // this is to tell the players that the process is finished
  PORTA = PORTA = ~SNES_UP;
  delay(delayTime);
  PORTA = PORTA = 0xFF;
  PORTA = PORTA = ~SNES_DOWN;
  delay(delayTime);
  PORTA = PORTA = 0xFF;
  PORTA = PORTA = ~SNES_LEFT;
  delay(delayTime);
  PORTA = PORTA = ~0xFF;
  PORTA = PORTA = ~SNES_RIGHT;
  delay(delayTime);
  PORTA = PORTA = 0xFF;
  outputState = lastOutputState = 0x0000;
  player1State = player2State = 0x0000;
}
// ==========================================================================
//                       Communicating with the SNES('s)
// ==========================================================================
void writeToChip(uint16_t data, int writtingMode) {
  if (writtingMode == PLAYER_ONE) {
    PORTA = ~(byte)data;
    PORTC = ~((data >> 8) | 0x00);
  }
  else if (writtingMode == PLAYER_TWO) {
    PORTL = ~(byte)data;
    PORTK = ~((data >> 8) | 0x00);
  }
}


void checkSelect() {
  /*
     This method enters into the selection mode if the
     select button of the first players controller is
     held down for selectPressTime
  */
  // check to see if the select buttons state has changed
  if (outputState & SNES_SELECT && !(lastOutputState & SNES_SELECT)) {
    // if it is pressed and the last reading it was not pressed
    // set lastSelectPress to now
    lastSelectPress = millis();
  }
  else if (outputState & SNES_SELECT &&
           lastOutputState & SNES_SELECT &&
           millis() > lastSelectPress + selectPressTime) {
    // press start and enter selection mode
    moveJoystick(100);
    mode = SELECTION_MODE;
  }
}
// ==========================================================================
//                               Modes of Operation
// ==========================================================================
uint16_t playerTakeover() {
  player1State = snes1.buttons();
  player2State = snes2.buttons();

  if (player1State & TRIGGER_MASK) {
    player2State = player1State;
  }

  if (player2State & TRIGGER_MASK) {
    player1State = player2State;
  }
  if (DEBUG) {
    Serial.print("PLAYER TAKEOVER : ");
  }
  // future improvement :
  // if one player has both trigger held down their controls replace the opponent
  // for 100 ms, then their controls are mixed with their opponents controls for 900ms
}

uint16_t bothControl() {
  /*
     This function returns any buttons pressed by either controller
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  output = player1State | player2State;
  if (DEBUG) {
    if (mode == PLAYERS_BOTH_CONTROL){
      Serial.print("BOTH CONTROL : ");
    }
    else if (mode == TWO_CONTROL_TWO){
      Serial.print("TWO CONTROL TWO : ");
    }
    printBits(player1State);
    Serial.print("-");
    printBits(player2State);
    Serial.print("-OUTPUT-");
    printBits(output);
    Serial.println(" ");
  }
  return output;
}

uint16_t playersAgree() {
  /*
     In this method the two input controllers are read
     their button states are compared using boolean AND
     and the results of the boolean operation are saved to state
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  output = player1State & player2State;

  if (DEBUG) {
    Serial.print("PLAYERS AGREE : ");
    printBits(player1State);
    Serial.print("-");
    printBits(player2State);
    Serial.print("-OUTPUT-");
    printBits(output);
    Serial.println(" ");
  }

  return output;
}

uint16_t playersDiffer() {
  /*
     This method reads both players controller
     and performs an XOR boolean comparison between the
     two controllers. The results are returned as an uint16_t
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  output = player2State ^ player1State;
  if (DEBUG) {
    Serial.print("PLAYERS DIFFER : ");
    printBits(player1State);
    Serial.print("-");
    printBits(player2State);
    Serial.print("-OUTPUT-");
    printBits(output);
    Serial.println(" ");
  }

  return output;
}

int playersDiverge() {
  /*
   * 
   */

  if ( divergeTurnLength > 3000 || divergeTurnLength < 0) {
    divergeDirection *= -1;
  }
  divergeTurnLength = divergeTurnLength + divergeDirection;   
  return divergeTurnLength;
}

uint16_t playersAlternateRandom(long mTurnLength) {
  /*
     This method checks to see who's "turn" it is
     If it is a players turn their controller is read
     and the results are outputed without any manipulation
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  if (millis() > turnStart + mTurnLength) {
    activePlayer = (activePlayer + 1) % NUM_PLAYERS;
    turnStart = millis();
    if (DEBUG) {
      Serial.print("Player");
      Serial.println(activePlayer + 1);
    }
    turnLength = mTurnLength * random(200) * 0.01 + (mTurnLength * 0.1);

  }
  if (activePlayer == 0) {
    output = player1State;
  }
  else if (activePlayer == 1) {
    output = player2State;
  }
  if (DEBUG) {
    Serial.print("Players Alernating Random : ");
    printBits(player1State);
    Serial.print("-");
    printBits(player2State);
    Serial.print("-OUTPUT-");
    printBits(output);
    Serial.println(" ");

  }
  return output;
}

uint16_t playersAlternate(long mTurnLength) {
  /*
     This method checks to see who's "turn" it is
     If it is a players turn their controller is read
     and the results are outputed without any manipulation
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  if (millis() > turnStart + mTurnLength) {
    activePlayer = (activePlayer + 1) % NUM_PLAYERS;
    turnStart = millis();
    if (DEBUG) {
      Serial.print("Player : ");
      Serial.println(activePlayer + 1);
    }
  }
  if (activePlayer == 0) {
    output = player1State;
  }
  else if (activePlayer == 1) {
    output = player2State;
  }

  if (DEBUG) {
    Serial.print("PLAYERS ALTERNATING : ");
    printBits(player1State);
    Serial.print("-");
    printBits(player2State);
    Serial.print("-OUTPUT-");
    printBits(output);
    Serial.println(" ");
  }
  return output;
}

// ------------ Printing and DeBugging ----------------

void printBits(uint16_t myByte) {
  for (uint16_t mask = 0x8000; mask; mask >>= 1) {
    if (mask  & myByte)
      Serial.print('1');
    else
      Serial.print('0');
  }
}
