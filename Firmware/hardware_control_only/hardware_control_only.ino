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
          3                 Green           xxxxx
          4                 Green           xxxxx
          5                 Green           xxxxx
          6                 Green           xxxxx
          7                 Green           xxxxx
          8                 Green           xxxxx

      If you want to play normal without any interfierence from the interface turn the switch off

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
#define TRIGGER_MASK 0xC00 //bitmask for both of the triggers

// ------------------------ MODES ----------------------------
//-----------  single player --------------------------------
#define SP_BOTH_CONTROL 0x05       // (OR) both players signal gets passed w/o resistance
#define SP_TEAMWORK 0x09              // (AND) both players have to press a button
#define SP_AGREE 0x11           // (XOR) if both players press a button it does not go through
#define SP_ALTERNATE 0x21       // players take turns of random length
#define SP_ALTERNATE_PRESSES 0x41  // (XOR) messages only pass if it is not present in both controllers
#define SP_TAKE_CONTROL 0x81          // players take turns
// ------------ Multiplayer ----------------------------------
#define MP_NORMAL1 0x06            // both controllers are passes as normal
#define MP_NORMAL2 0x0A            // both controllers are passes as normal
#define MP_XOR 0x12                // (XOR) if both players press a button it does not go through
#define MP_TAKE_CONTROL 0x22       // by pressing both triggers you switch the character you are controllling
#define MP_ALTERNATE_CONTROL 0x42  // players alternate whom controls both characters
#define MP_BOTH_CONTROL 0x82       // both players control both characters, really silly

// --------------------- Controlers and Players -------------------

#define PLAYER_ONE 1
#define PLAYER_TWO 2

// ================================================================
//                            Globals
// ================================================================
// for dev and debug
uint8_t DEBUG = 5;
/*
   0 is no DEBUG
   1 is regular DEBUG - adds print statements
   2 is same as 1 but adds delay to program
*/
uint8_t mode = MP_BOTH_CONTROL;
// dealing with which player is active as well
// as turn durations and lengths
uint8_t activePlayer = PLAYER_ONE;
uint32_t turnStart = 0;
uint32_t pastPoll = 0;
uint32_t turnLength = 0;
uint8_t turnLengthScaler = 4;

uint32_t selectPressTime = 1500;
uint32_t lastSelectPress = 0;

uint64_t nextRandom = 0;

// LED flashing
uint64_t lastFlash = 0;
uint64_t lastLedBlink;

// to keep track of last output states
uint16_t p1LastOutputState = 0x0FFF;
uint16_t p1OutputState = 0x0000;
uint16_t p2LastOutputState = 0x0FFF;
uint16_t p2OutputState = 0x0000;
uint32_t p1p2OutputState = 0;
// past button states
uint16_t player1State = 0x0000;
uint16_t player2State = 0x0000;

// POT
uint16_t potVal;
uint16_t lastPotVal;

// Rotary
uint8_t rotaryState;
uint8_t lastRotaryState;

// PLAYERS_TAKE_CONTROL play modes
uint64_t p1LastTrigger;
uint64_t p2LastTrigger;
uint16_t triggerDebouce = 1000;
bool outputsSwitched = false;
bool lastOutputsSwitched = false;
uint16_t tc_temp;

// ALTERNATE_PRESSES play modes
uint16_t maxPresses;

// TEAMWORK mode
uint8_t simPress;
uint16_t p1OutputMask;
uint16_t p2OutputMask;
uint16_t p1LastOutputMask;
uint16_t p2LastOutputMask;

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

  pinMode(POT_SIG, INPUT);
  // set controller ports to HIGH (the resting state)
  PLAYER1_BUTTONS1 = 0xFF;
  PLAYER1_BUTTONS2 = 0xFF;
  PLAYER2_BUTTONS1 = 0xFF;
  PLAYER2_BUTTONS2 = 0xFF;
  // set LED's port to LOW
  STATUS_LEDS = 0x00;

  delay(10);
  Serial.begin(57600);
  Serial.println("SERIAL BUS OPENED");
}

void loop() {
  mode = readRotary();
  // hardware pullup reset
  ROTARY_AND_SWITCH = 0xFF;
  turnLength = maxPresses =  readPot();

  switch (mode) {
    default:
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      writeToChip(p1OutputState, p2OutputState);
      dprintState("DEFAULT MODE :", p1OutputState, p2OutputState);
      break;

    case SP_BOTH_CONTROL:
      p1OutputState = p2OutputState = bothControl();
      writeToChip(p1OutputState, p2OutputState);
      break;

    case SP_AGREE:
      p1OutputState = p2OutputState = playersAgree();
      writeToChip(p1OutputState, p2OutputState);
      break;

    case SP_ALTERNATE:
      p1OutputState = p2OutputState = playersAlternate(turnLength);
      writeToChip(p1OutputState, p2OutputState);
      break;

    case SP_TAKE_CONTROL:
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      dprint(outputsSwitched);
      dprintState(" SP TAKE CONTROL : ", p1OutputState, p2OutputState);
      if ((p1OutputState & TRIGGER_MASK) == TRIGGER_MASK) {
        if (millis() > p1LastTrigger + triggerDebouce) {
          p1LastTrigger = millis();
          lastOutputsSwitched = outputsSwitched;
          outputsSwitched = !outputsSwitched;
        }
      }
      else if ((p2OutputState & TRIGGER_MASK) == TRIGGER_MASK) {
        if (millis() > p2LastTrigger + triggerDebouce) {
          p2LastTrigger = millis();
          lastOutputsSwitched = outputsSwitched;
          outputsSwitched = !outputsSwitched;
        }
      }
      if (outputsSwitched == true) {
        if (p1LastTrigger > p2LastTrigger) {
          p2OutputState = p1OutputState;
          if (millis() > p1LastTrigger + (potVal * 3) + 500) {
            outputsSwitched = false;
          }
        }
        else {
          p1OutputState = p2OutputState;
          if (millis() > p2LastTrigger + (potVal * 3) + 500) {
            outputsSwitched = false;
          }
        }
      }
      else {
        p1OutputState = p2OutputState = p1OutputState | p2OutputState;
      }

      lastOutputsSwitched = outputsSwitched;
      writeToChip(p1OutputState, p2OutputState);
      break;

    case SP_ALTERNATE_PRESSES:
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      static uint16_t press_count;
      maxPresses = (int)(potVal / 15);
      if (press_count > maxPresses) {
        activePlayer = (activePlayer + 1) % 2;
        press_count = 0;
      }
      if (activePlayer == 0) {
        if (p1OutputState > p1LastOutputState) {
          press_count++;
        }
        p2OutputState = p1OutputState;
      }
      else if (activePlayer == 1) {
        if (p2OutputState > p2LastOutputState) {
          press_count++;
        }
        p1OutputState = p2OutputState;
      }
      writeToChip(p1OutputState, p2OutputState);
      dprint(press_count);
      dprint("/");
      dprint(maxPresses);
      dprint(" ");
      dprint(activePlayer);
      dprintState(" SP ALT PRESSES :", p1OutputState, p2OutputState);
      break;

    case SP_TEAMWORK:
      p1LastOutputState = p1OutputState;
      p2LastOutputState = p2OutputState;
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      
      if (p1OutputState ^ p1LastOutputState != 0 && p1OutputState > p1LastOutputState) {
        p1LastOutputMask = p1OutputMask;
        p1OutputMask = (p1OutputState ^ p1LastOutputState);
        if (countBits(p1OutputMask) > 1) {
          p1OutputMask = buttonSettle(p1OutputMask ^ p1LastOutputMask);
        }
      }
      else if (p1OutputState ^ p1LastOutputState > 0) {
        if (countBits(p1OutputState) < countBits(p1LastOutputState)) {
          if (p1LastOutputMask & p1OutputState == 0) {
            p1OutputMask = buttonSettle(p1OutputState);
          }
          else {
            p1OutputMask = buttonSettle(p1LastOutputMask); 
          }
          p1LastOutputMask = p1OutputMask;
        }
      }
      if (p2OutputState ^ p2LastOutputState != 0 && p2OutputState > p2LastOutputState) {
        p2LastOutputMask = p2OutputMask;
        p2OutputMask = (p2OutputState ^ p2LastOutputState);
        if (countBits(p2OutputMask) > 1) {
          p2OutputMask = buttonSettle(p2OutputMask ^ p2LastOutputMask);
        }
      }
      else if (p2OutputState ^ p2LastOutputState > 0) {
        if (countBits(p2OutputState) < countBits(p2LastOutputState)) {
          if (p2LastOutputMask & p2OutputState == 0) {
            p2OutputMask = buttonSettle(p2OutputState);
          }
          else {
            p2OutputMask = buttonSettle(p2LastOutputMask); 
          }
          p2LastOutputMask = p2OutputMask;
        }
      }
      // the AND must not be directly applied to the p1OutputState else algorithm will fail
      writeToChip(p1OutputState | p2OutputState, p2OutputState | p1OutputState);
      dprintState("SP TEAMWORK :", p1OutputState, p2OutputState);
      break;

    case MP_NORMAL1:
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      writeToChip(p1OutputState, p2OutputState);
      dprintState("MP NORMAL 2 :", p1OutputState, p2OutputState);
      break;

    case MP_NORMAL2:
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      writeToChip(p1OutputState, p2OutputState);
      dprintState("MP NORMAL 1 :", p1OutputState, p2OutputState);
      break;

    case MP_XOR:
      static uint16_t temp_mask;
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      temp_mask = p1OutputState & p2OutputState;
      p1OutputState = p1OutputState ^ temp_mask;
      p2OutputState = p2OutputState ^ temp_mask;
      writeToChip(p1OutputState, p2OutputState);
      dprintState("MP XOR :", p1OutputState, p2OutputState);
      break;

    case MP_TAKE_CONTROL:
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      dprint(outputsSwitched);
      dprintState(" MP TAKE CONTROL : ", p1OutputState, p2OutputState);
      if ((p1OutputState & TRIGGER_MASK) == TRIGGER_MASK) {
        if (millis() > p1LastTrigger + triggerDebouce) {
          p1LastTrigger = millis();
          lastOutputsSwitched = outputsSwitched;
          outputsSwitched = !outputsSwitched;
        }
      }
      else if ((p2OutputState & TRIGGER_MASK) == TRIGGER_MASK) {
        if (millis() > p2LastTrigger + triggerDebouce) {
          p2LastTrigger = millis();
          lastOutputsSwitched = outputsSwitched;
          outputsSwitched = !outputsSwitched;
        }
      }
      if (outputsSwitched == true) {
        tc_temp = p1OutputState;
        p1OutputState = p2OutputState;
        p2OutputState = p1OutputState;
        if (p1LastTrigger > p2LastTrigger & millis() > p1LastTrigger + (potVal * 2) + 400) {
          outputsSwitched = false;
        }
        else if (millis() > p2LastTrigger + (potVal * 2) + 400) {
          outputsSwitched = false;
        }
      }
      lastOutputsSwitched = outputsSwitched;
      writeToChip(p1OutputState, p2OutputState);
      break;

    case MP_ALTERNATE_CONTROL:
      static uint16_t alt_mask;
      alt_mask = playersAlternate(turnLength);
      p1OutputState = snes1.buttons() | alt_mask;
      p2OutputState = snes2.buttons() | alt_mask;
      writeToChip(p1OutputState, p2OutputState);
      dprint("MP : ");
      break;

    case MP_BOTH_CONTROL:
      static uint16_t bc_mask;
      bc_mask = bothControl();
      p1OutputState = snes1.buttons() | bc_mask;
      p2OutputState = snes2.buttons() | bc_mask;
      writeToChip(p1OutputState, p2OutputState);
      dprint("MP : ");
      break;
  }

  // testter();
  flashLeds();
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

uint16_t playersAlternate(uint16_t mTurnLength) {
  /*
     This method checks to see who's "turn" it is
     If it is a players turn their controller is read
     and the results are outputed without any manipulation
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  if (millis() > turnStart + turnLengthScaler * potVal) {
    activePlayer = (activePlayer + 1) % 2;
    turnStart = millis();
  }
  if (activePlayer == 0) {
    output = player1State;
  }
  else if (activePlayer == 1) {
    output = player2State;
  }
  if (DEBUG) {
    String status_string = "PLAYERS ALTERNATING : P";
    status_string = status_string + activePlayer + " ";
    dprintState(status_string, player1State, player2State);
  }
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
  if (mode == SP_TEAMWORK) {
    dprint(" TW OUTPUTS : ");
    printBits((p1OutputState | p2OutputState) & (p1OutputMask | p2OutputMask));
    dprint(" : ");
    printBits((p2OutputState | p1OutputState) & (p1OutputMask | p2OutputMask));
    dprintln(" ");
  }
  else {
    dprint(" OUTPUTS : ");
    printBits(p1OutputState);
    dprint(" : ");
    printBits(p2OutputState);
    dprintln(" ");
  }
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
  if (mode == SP_ALTERNATE_PRESSES | mode == SP_ALTERNATE) {
    if (activePlayer == 0) {
      STATUS_LEDS = mode;
    }
    else {
      STATUS_LEDS = mode ^ 0x01;
    }
  }
  else if (mode == MP_TAKE_CONTROL) {

    if (outputsSwitched != lastOutputsSwitched) {
      if (millis() > p1LastTrigger + 300) {
        STATUS_LEDS = mode ^ 0x02;
      }
      else if (millis() > p2LastTrigger + 300) {
        STATUS_LEDS = mode ^ 0x02;
      }
    }
    else {
      STATUS_LEDS = mode;
    }

  }
  else if (mode == MP_ALTERNATE_CONTROL) {
    if (activePlayer == 0) {
      STATUS_LEDS = mode;
    }
    else {
      STATUS_LEDS = mode ^ 0x02;
    }
  }
  else {
    STATUS_LEDS = mode;
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
    if (mode == SP_TEAMWORK) {
      PLAYER1_BUTTONS1 = ~(byte)(player1 & (p1OutputMask | p2OutputMask));
      PLAYER1_BUTTONS2 = ~(((player1 & (p1OutputMask | p2OutputMask)) >> 8) | 0x00);
    }
    else {
      PLAYER1_BUTTONS1 = ~(byte)player1;
      PLAYER1_BUTTONS2 = ~((player1 >> 8) | 0x00);
    }
    p1LastOutputState = player1;
  }
  if (p2LastOutputState != player2) {
    if (mode == SP_TEAMWORK) {
      PLAYER2_BUTTONS1 = ~(byte)(player2 & (p1OutputMask | p2OutputMask));
      PLAYER2_BUTTONS2 = ~(((player2 & (p1OutputMask | p2OutputMask)) >> 8) | 0x00);
    }
    else {
      PLAYER2_BUTTONS1 = ~(byte)player2;
      PLAYER2_BUTTONS2 = ~((player2 >> 8) | 0x00);
    }
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

uint16_t buttonSettle(uint16_t myByte) {
  for (uint16_t mask = 0x8000; mask; mask >>= 1) {
    if (mask & myByte)
      return mask;
  }
}

uint8_t countBits(uint16_t myByte) {
  uint8_t bits = 0;
  for (uint16_t mask = 0x8000; mask; mask >>= 1) {
    if (mask & myByte) {
      bits += 1;
    }
  }
  return bits;
}

void dprint(String msg) {
  if (DEBUG) {
    Serial.print(msg);
  }
}

void dprint(int msg) {
  if (DEBUG) {
    if (msg >= 0) {
      if (msg < 1000) {
        Serial.print(" ");
        if (msg < 100) {
          Serial.print(" ");
          if (msg < 10) {
            Serial.print(" ");
          }
        }
      }
    }
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

void testter() {
  uint16_t on = 0xff;
  uint16_t off = 0x00;
  writeToChip(on, on);
  delay(700);
  writeToChip(off, off);
  delay(700);
}

