/*
    Arduino Firmware for interfacing with the SNES

    Two Input Controllers : Two Output Controllers

    Programmed by Nathan Villicana-Shaw
    Fall 2015

    This project is a work in progress and builds upon the shoulders
    of many brilliant programmers, hackers, engineers and artists
    (links to be added soon)

    This specific version is for use with the Arduino Shield V1.1
    All the pinouts used in this configuration will be inside the () next to
    the normal values


    ===========================================================
    ARDUINO : PORTS AND PINS
    ===========================================================

    -------------------------------------------------
    PLAYER 1 : INPUT
    -------------------------------------------------
    GND
    5v
    DATA          5
    LATCH         3
    CLOCK         2
    -------------------------------------------------
    PLAYER 2 : INPUT
    -------------------------------------------------
    GND
    5v
    DATA          8
    LATCH         7
    CLOCK         6
    -------------------------------------------------
    PLAYER 1 : OUTPUT
    -------------------------------------------------
    PORTA on the Arduino Mega consists of pins 22-29
      22 is bit 0, 29 is bit 7
      we use all the pins  (11111111)

    PORTC on the Arduino MEGA consists of pins 30-37
    `37 is bit 0, 30 is bit 7
      we use pins 34-37 (xxxx1111)

    --------------------------------------------------
    PLAYER 2 : OUTPUT
    -------------------------------------------------
    PORTL : on the Arduino Mega consists of pins 42-49
      49 is bit 0, 42 is bit 7
      we use all the pins (11111111)

    PORTK : on the Arduino Mega consists of analog pins a8-a15
      a8 is bit 0, a15 is bit 7
      we use pins a8-a15 (xxxx1111)

    --------------------------------------------------
    LEDS :
    --------------------------------------------------
    PORTF : on the Arduno Mega port F consists of analog
      pins 0 - 7, each LED corrisponds with an operation mode

    ================================================================================
                                  SNES CONTROLLERS
    ================================================================================

    It is very important to note that the SNES controller reads a button as being pressed
    upon the grounding of its corrisponding pin -  Thus when a 0 is written to a Pin the
    button is "pressed" until the pin is raised back to its HIGH

    When hooking up the SNES controllers there are a few things that help the process :

                  SNES Controller Jack Pins
     ----------------------------- ---------------------
    |                             |                      \
    | (1)     (2)     (3)     (4) |   (5)     (6)     (7) |
    |                             |                      /
     ----------------------------- ---------------------

    Pin   Description   Wire Color(OEM)  Wire Color(controller extenders)
    ===   ===========   ==============   ==================
     1    +5v (power)   White            Blue
     2    Clock         Yellow           Yellow
     3    Latch         Orange           Black
     4    Data          Red              Red
     5    Nothing       None             None
     6    Nothing       None             None
     7    Ground        Brown            Green

     Data Packing Order for the buttons. Note that the first clock cycle
     is acutually the downwards edge of the LATCH

     Clock Cycle     Button Reported          MEGA PIN Player 1         MEGA PIN Player 2
     ===========     ===============          =================         =================
        1               B                           22                          49
        2               Y                           23                          48
        3               Select                      24                          47
        4               Start - down                25                          46
        5               Up on joypad                26                          45
        6               Down on joypad              27                          44
        7               Left on joypad              28                          43
        8               Right on joypad             29                          42
        9               A                           37                          A8
        10              X                           36                          A9
        11              L                           35                          A10
        12              R                           34                          A11
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
      =======================================================================
                                      LEDS
      =======================================================================
      for expandability and to allow for all the currently available modes of operation
      different color LEDS will be used to help provide more meaningful user feedback
      Control for 8LED's total is provided by the shield

        LED Num          LED Color          Meaning
       (on Board)

          1                 Red             If on it Denotes a VS mode
          2                 Blue            If on it Denotes a Co-Op Mode
          3                 Green           Corrisponds with mode (multiple can be on to allow for more than 6 modes)
          4                 Green           Corrisponds with mode (multiple can be on to allow for more than 6 modes)
          5                 Green           Corrisponds with mode (multiple can be on to allow for more than 6 modes)
          6                 Green           Corrisponds with mode (multiple can be on to allow for more than 6 modes)
          7                 Green           Corrisponds with mode (multiple can be on to allow for more than 6 modes)
          8                 Green           Corrisponds with mode (multiple can be on to allow for more than 6 modes)

      All of the LEDS will flash when the game is in the selection mode

      =======================================================================
                                    General Notes
      =======================================================================

      For the SNES controllers the buttons resting position is HIGH as 5v when
      the pad is grounded the button is pressed. This is usally done by bridging
      pads together using conductive plastic that rests on the bottom of the button
      you pressed. In this project we solder wires to the sensing pads that we ground
      using our arduinos digital pins.

      TODO ::

      Flushout the player takeover system
        - perhaps the buttons become the other players D-pad?

       Have different color LEDS for 'single player' and multiplayer modes

       Add logic for the pot: change turn length, and other things
*/
// ================================================================
//                           Preprocessor
// ================================================================

#include <SNESpad.h>

// the pins we use for receiving data from the two SNES controllers

// --------------------- Arduino Pins ------------------------

#define SHIELD_VERSION 2 // 0 is no shield proto, 1 is v1 of shield, 2 is v1.1 of shield

#if SHIELD_VERSION == 2
#define IN1_LATCH 3
#define IN1_CLOCK 2
#define IN1_DATA 5
#define IN2_LATCH 7
#define IN2_CLOCK 6
#define IN2_DATA 8
#define POT_SIG A15
#endif

#if SHIELD_VERSION == 1
#define IN1_LATCH 12
#define IN1_CLOCK 13
#define IN1_DATA 11
#define IN2_LATCH 50
#define IN2_CLOCK 10
#define IN2_DATA 51
#define POT_SIG A15
#endif

#if SHIELD_VERSION == 0
#define IN1_LATCH 2
#define IN1_CLOCK 3
#define IN1_DATA 4
#define IN2_LATCH 5
#define IN2_CLOCK 6
#define IN2_DATA 7
#define POT_SIG A15
#endif

// --------------------- PORTS -----------------------------

#define PLAYER1_BUTTONS1 PORTA
#define PLAYER1_BUTTONS2 PORTC
#define PLAYER2_BUTTONS1 PORTL
#define PLAYER2_BUTTONS2 PORTK
#define ROTARY_AND_SWITCH PORTB
#define STATUS_LEDS PORTF

// --------------------- MASKS -------------------------------

#define MODE_SELECT_MASK 0x0F03
#define TRIGGER_MASK 0x0C //bitmask for both of the triggers

// ------------------------ MODES ----------------------------
// single player modes
#define SP_BOTH_CONTROL 0x05       // (OR) both players signal gets passed w/o resistance
#define SP_AGREE 0x09              // (AND) both players have to press a button
#define SP_ALTERNATE 0x11          // players take turns
#define SP_ALTERNATE_RANDOM 0x21   // players take turns of random length
#define SP_DIVERGE 0x41            // (XOR) messages only pass if it is not present in both controllers
#define SP_DIFFER 0x81             // (XOR) if both players press a button it does not go through
// multiplayer modes
#define MP_BOTH_CONTROL 0x06       // (OR) both players signal gets passed w/o resistance
#define MP_AGREE 0x0A              // (AND) both players have to press a button
#define MP_ALTERNATE 0x12          // players take turns
#define MP_ALTERNATE_RANDOM 0x22   // players take turns of random length
#define MP_DIVERGE 0x42            // (XOR) messages only pass if it is not present in both controllers
#define MP_DIFFER 0x82             // (XOR) if both players press a button it does not go through

// --------------------- Controlers and Players -------------------

// for chip write modes
#define PLAYER_ONE 1
#define PLAYER_TWO 2

// ================================================================
//                            Globals
// ================================================================
// for dev and debug
uint8_t DEBUG = 1;
/*
   0 is no DEBUG
   1 is regular DEBUG - adds print statements
   2 is same as 1 but adds delay to program
*/
uint8_t mode = SP_AGREE;
// dealing with which player is active as well
// as turn durations and lengths
uint8_t activePlayer = PLAYER_ONE;
uint32_t turnStart = 0;
uint32_t pastPoll = 0;
uint32_t turnLength = 4;

uint32_t selectPressTime = 1500;
uint32_t lastSelectPress = 0;

// for helping with LED flashing
uint32_t lastFlash = 0;
uint16_t fastFlash = 100;
uint16_t slowFlash = 400;

uint32_t divergeTurnLength = 1;
int8_t divergeDirection;
uint16_t maxDivergeTurnLength = 5000;

// for helping with mode selections
uint32_t lastLeftTriggerPress;
uint32_t lastRightTriggerPress;
uint32_t triggerPressTime = 2000;
uint32_t lastLedBlink;

// to keep track of last output sttes
uint16_t p1LastOutputState = 0x0FFF;
uint16_t p1OutputState = 0x0000;
uint16_t p2LastOutputState = 0x0FFF;
uint16_t p2OutputState = 0x0000;
uint32_t p1p2OutputState = 0;

// variable to keep track of past button states
uint16_t player1State = 0x0000;
uint16_t player2State = 0x0000;

// variables for keeping track of the POT
uint16_t potVal;
uint16_t lastPotVal;

// variables for keeping track of the Rotary
uint8_t rotaryState;
uint8_t lastRotaryState;

// create instances of our SNES controller reader objects
SNESpad snes1 = SNESpad(IN1_LATCH, IN1_CLOCK, IN1_DATA);
SNESpad snes2 = SNESpad(IN2_LATCH, IN2_CLOCK, IN2_DATA);

// =================================================================
//                      Setup and Loop
// =================================================================

void setup() {
  // set controller ports to outputs
  DDRA = 0xFF;
  DDRC = 0xFF;
  DDRL = 0xFF;
  DDRK = 0xFF;
  // set LED's port to output
  DDRF = 0xFF;
  // turn on pullup resistors for rotary
  ROTARY_AND_SWITCH = 0xFF;
  // set the ROTARY INS as inputs
  DDRB = 0x00;
  // sloppy troubleshooting
  // set pot_pin to input
  pinMode(POT_SIG, INPUT);
  // set controller ports to HIGH (the resting state)
  PLAYER1_BUTTONS1 = 0xFF;
  PLAYER1_BUTTONS2 = 0xFF;
  PLAYER2_BUTTONS1 = 0xFF;
  PLAYER2_BUTTONS2 = 0xFF;
  // set LED's port to LOW
  STATUS_LEDS = 0x00;
  // let things settle
  delay(10);

  if (DEBUG) {
    Serial.begin(57600);
    Serial.println("SERIAL BUS OPENED");
  }
}

void loop() {
  switch (mode) {
    default:
      Serial.print("Error mode not detected : ");
      Serial.print(mode);
      Serial.print(" ");
      mode = SP_BOTH_CONTROL;
      
    case MP_AGREE:
      p1OutputState = p2OutputState = playersAgree();
      writeToChip(p1OutputState, p2OutputState);
      break;
      
    case SP_AGREE:
      p1OutputState = p2OutputState = playersAgree();
      writeToChip(p1OutputState, p2OutputState);
      break;

    case MP_ALTERNATE:
      p1OutputState = p2OutputState = playersAlternate(turnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;
    
    case SP_ALTERNATE:
      p1OutputState = p2OutputState = playersAlternate(turnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;
      
    case MP_ALTERNATE_RANDOM:
      p1OutputState = p2OutputState = playersAlternateRandom(turnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;

    case SP_ALTERNATE_RANDOM:
      p1OutputState = p2OutputState = playersAlternateRandom(turnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;
    
    case MP_DIFFER:
      p1OutputState = p2OutputState = playersDiffer();
      writeToChip(p1OutputState, p2OutputState);
      break;
      
    case SP_DIFFER:
      p1OutputState = p2OutputState = playersDiffer();
      writeToChip(p1OutputState, p2OutputState);
      break;

    case MP_BOTH_CONTROL:
      p1OutputState = p2OutputState = bothControl();
      writeToChip(p1OutputState, p2OutputState);
      break;

    case SP_BOTH_CONTROL:
      p1OutputState = p2OutputState = bothControl();
      writeToChip(p1OutputState, p2OutputState);
      break;

    case MP_DIVERGE:
      divergeTurnLength = playersDiverge();
      p1OutputState = p2OutputState = playersAlternate(divergeTurnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;
      
    case SP_DIVERGE:
      divergeTurnLength = playersDiverge();
      p1OutputState = p2OutputState = playersAlternate(divergeTurnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;

  }
  
  flashLeds();
  mode = readRotary();
  ROTARY_AND_SWITCH = 0xFF;
  p1LastOutputState = p1OutputState;
  p2LastOutputState = p2OutputState;
  readPot();
  mainLoopDebug();
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
  dprint("PLAYER TAKEOVER : ");
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
  dprintState("BOTH CONTROL : ", player1State, player2State);
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
  dprintState("PLAYERS AGREE :", player1State, player2State);  
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
  dprintState("PLAYERS DIFFER : ", player1State, player2State);
  return output;
}

int playersDiverge() {
  if ( divergeTurnLength > maxDivergeTurnLength || divergeTurnLength < 0) {
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
  if (millis() > turnStart + ((mTurnLength + (random(-1,1) * mTurnLength * 0.75) * potVal)) {
    activePlayer = (activePlayer + 1) % 2;
    turnStart = millis();
    )
  }

  if (activePlayer == 0) {
    output = player1State;
  }
  else if (activePlayer == 1) {
    output = player2State;
  }
  String state = "PLAYERS ALTERNATING RANDOM : P";
  state = state + activePlayer + " " + "turnLength : " + turnLength + " ";
  dprintState(state, player1State, player2State);
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
  if (millis() > turnStart + mTurnLength * potVal) {
    activePlayer = (activePlayer + 1) % 2;
    turnStart = millis();
  }
  if (activePlayer == 0) {
    output = player1State;
  }
  else if (activePlayer == 1) {
    output = player2State;
  }
  String status_string = "PLAYERS ALTERNATING : P";
  status_string = status_string + activePlayer + " ";
  dprintState(status_string, player1State, player2State);
  return output;
}

// ==========================================================================
// Helper Functions for Modes of Operations
// ==========================================================================

void mainLoopDebug() {
  dprint(" pv : ");
  dprint(potVal);
  if (DEBUG > 5) {
    dprint(" : Rotary State : ");
    for (int i = 0; i < 8; i++) {
      dprint((rotaryState << i) & 0x01);
    }
    dprint(rotaryState);
  }
  if (potVal < 1000) {
    dprint(" ");
    if (potVal < 100) {
      dprint(" ");
      if (potVal < 10) {
        dprint(" ");
      }
    }
  }
  dprint(" : ");
  if (DEBUG > 1) {
    delay(10 * DEBUG);
  };
  dprint(" OUTPUTS : ");
  printBits(p1OutputState);
  dprint(" : ");
  printBits(p2OutputState);
  dprintln(" ");
}

uint16_t readPot() {
  lastPotVal = potVal;
  potVal = analogRead(POT_SIG);
  return potVal;
}

uint8_t readRotary() {
  lastRotaryState = rotaryState;
  rotaryState = ~PINB;
  return rotaryState;
}

void flashLeds() {
  if (mode != 0) {
    STATUS_LEDS = mode;
  }
  else if (millis() > lastFlash + slowFlash) {
    STATUS_LEDS = ~STATUS_LEDS;
    lastFlash = millis();
  }
}

int determineOutputController() {
  /*
       This method allows a single player to toggle
       their signal's output between the two output
       controllers

       this method only responds to the first input controller
       as it assumes you are playing alone
  */
  // check to see if the trigger buttons state has changed
  if (~PLAYER1_BUTTONS2 & TRIGGER_MASK && p1LastOutputState & TRIGGER_MASK) {
    // if it is pressed and the last reading it was not pressed
    // set lastSelectPress to now
    lastSelectPress = millis();
  }
  // if they are off but were just turned on
  else if (!(~PLAYER1_BUTTONS2 & TRIGGER_MASK) && !(p1LastOutputState & TRIGGER_MASK)) {
    // do something
  }
  // if the trigger buttons are held down for select press time flip the activePlayer
  else if (~PLAYER1_BUTTONS2 & TRIGGER_MASK &&
           p1LastOutputState & TRIGGER_MASK &&
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

// ==========================================================================
//                       Communicating with the SNES('s)
// ==========================================================================

void writeToChip(uint16_t data, uint8_t writtingMode) {
  if (writtingMode == PLAYER_ONE) {
    if (p1LastOutputState != data) {
      PLAYER1_BUTTONS1 = ~(byte)data;
      PLAYER1_BUTTONS2 = ~((data >> 8) | 0x00);
      p1LastOutputState = data;
    }
  }
  else if (writtingMode == PLAYER_TWO) {
    if (p2LastOutputState != data) {
      PLAYER2_BUTTONS1 = ~(byte)data;
      PLAYER2_BUTTONS2 = ~((data >> 8) | 0x00);
      p2LastOutputState = data;
    }
  }
}

void writeToChip(uint16_t player1, uint16_t player2) {
  if (p1LastOutputState != player1) {
    PLAYER1_BUTTONS1 = ~(byte)player1;
    PLAYER1_BUTTONS2 = ~((player1 >> 8) | 0x00);
    p1LastOutputState = player1;
  }
  if (p2LastOutputState != player2) {
    PLAYER2_BUTTONS1 = ~(byte)player2;
    PLAYER2_BUTTONS2 = ~((player2 >> 8) | 0x00);
    p2LastOutputState = player2;
  }
}

// ============================================================
// ------------ Printing and DeBugging ----------------
// ============================================================

void dprintState(String modeString, uint16_t player1State, uint16_t player2State) {
  if (DEBUG) {
    Serial.print(modeString);
    printBits(player1State);
    Serial.print("-");
    printBits(player2State);
    Serial.print(" - ");
  }
}

void printBits(uint16_t myByte) {
  for (uint16_t mask = 0x8000; mask; mask >>= 1) {
    if (mask & myByte)
      Serial.print('1');
    else
      Serial.print('0');
  }
}

void printBits(uint8_t myByte) {
  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    if (mask & myByte)
      Serial.print('1');
    else
      Serial.print('0');
  }
}

void dprint(String msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

void dprint(int msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

void dprintln(String msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}

void dprintln(int msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}
