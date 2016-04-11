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
#define TRIGGER_MASK1 0xC00 //bitmask for both of the triggers

// ------------------------ MODES ----------------------------
//-----------  single player --------------------------------
#define SP_BOTH_CONTROL 0x06          // (OR) both players signal gets passed w/o resistance
#define SP_TEAMWORK 0x0A              // (AND) both players have to press a button
#define SP_AGREE 0x12                 // (XOR) if both players press a button it does not go through
#define SP_ALTERNATE 0x22             // players take turns of random length
#define SP_ALTERNATE_PRESSES 0x42     // (XOR) messages only pass if it is not present in both controllers
#define SP_TAKE_CONTROL 0x82          // players take turns
// ------------ Multiplayer ----------------------------------
#define MP_PRESSED_FIRST 0x05      // both controllers are passes as normal
#define MP_LIMITED_PRESS 0x09      // both controllers are passes as normal
#define MP_XOR 0x11                // (XOR) if both players press a button it does not go through
#define MP_TAKE_CONTROL 0x21       // by pressing both triggers you switch the character you are controllling
#define MP_ALTERNATE_CONTROL 0x41  // players alternate whom controls both characters
#define MP_BOTH_CONTROL 0x81       // both players control both characters, really silly

// --------------------- Controlers and Players -------------------

#define PLAYER_ONE 1
#define PLAYER_TWO 2

// ================================================================
//                            Globals
// ================================================================

// uint8_t MODE = SP_TEAMWORK;
// uint8_t MODE = SP_BOTH_CONTROL;
uint8_t MODE = SP_AGREE;
// uint8_t MODE = SP_ALTERNATE;
// uint8_t MODE = SP_ALTERNATE_PRESSES;

// for dev and debug
uint8_t DEBUG = 1;
/*
   0 is no DEBUG
   1 is regular DEBUG - adds print statements
   2 is same as 1 but adds delay to program
*/

// dealing with which player is active as well
uint8_t activePlayer = PLAYER_ONE;
uint64_t turnStart = 0;
uint16_t turnLength = 0;
uint8_t turnLengthScaler = 4;

// LED flashing
uint64_t lastLedBlink;

// to keep track of last output states
uint16_t p1LastOutputState = 0x0FFF;
uint16_t p1OutputState = 0x0000;
uint16_t p2LastOutputState = 0x0FFF;
uint16_t p2OutputState = 0x0000;

// past button states
uint16_t player1State = 0x0000;
uint16_t player2State = 0x0000;

// VARIABLES Specific to Certain Modes

// ALTERNATE_PRESSES play modes
uint16_t maxPresses;

// TEAMWORK mode
uint8_t simPress;
uint16_t p1OutputMask, p2OutputMask;
uint16_t p1LastOutputMask, p2LastOutputMask;

// LIMITED PRESS
uint16_t p1Presses, p2Presses;
uint64_t lastPressCycle;
bool p1Does;
bool p2Does;

// XOR
uint16_t temp_mask, temp1, temp2;

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
  switch (MODE) {
    case SP_BOTH_CONTROL:
      spBothControl();
      break;

    case SP_AGREE:
      spPlayersAgree();
      break;

    case SP_ALTERNATE:
      spPlayersAlternate(turnLength);
      break;

    case SP_TAKE_CONTROL:
      spTakeControl();
      break;

    case SP_ALTERNATE_PRESSES:
      spAlternatePresses();
      break;

    case SP_TEAMWORK:
      spTeamwork();
      break;

    case MP_PRESSED_FIRST:
      mpPressedFirst();
      break;

    case MP_LIMITED_PRESS:
      mpLimitedPress();
      break;

    case MP_XOR:
      mpXOR();
      break;

    case MP_TAKE_CONTROL:
      mpTakeControl();
      break;

    case MP_ALTERNATE_CONTROL:
      mp_alternate_control();
      break;

    case MP_BOTH_CONTROL:
      mpBothControl();
      break;
  }
  flashLeds();
  mainLoopDebug();
}

// ==========================================================================
//                    Single PLayer Modes of Operation
// ==========================================================================

void spTeamwork() {

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
}

void spAlternatePresses() {
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
  dprintln(" ");
  dprint(press_count);
  dprint("/");
  dprint(maxPresses);
  dprint(" ");
  dprint(activePlayer);
  dprintState(" SP ALT PRESSES :", p1OutputState, p2OutputState);
}

void spTakeControl() {
  p1OutputState = snes1.buttons();
  p2OutputState = snes2.buttons();
  dprint(outputsSwitched);
  dprintState(" SP TAKE CONTROL : ", p1OutputState, p2OutputState);
  if ((p1OutputState & TRIGGER_MASK1) == TRIGGER_MASK1) {
    if (millis() > p1LastTrigger + triggerDebouce) {
      p1LastTrigger = millis();
      outputsSwitched = true;
    }
  }
  else if ((p2OutputState & TRIGGER_MASK1) == TRIGGER_MASK1) {
    if (millis() > p2LastTrigger + triggerDebouce) {
      p2LastTrigger = millis();
      outputsSwitched = true;
    }
  }
  if (outputsSwitched == true) {
    if (p1LastTrigger > p2LastTrigger) {
      p2OutputState = p1OutputState;
      if (millis() > p1LastTrigger + (potVal * 2) + 1500) {
        outputsSwitched = false;
        dprint("BING");
      }
    }
    else {
      p1OutputState = p2OutputState;
      if (millis() > p2LastTrigger + (potVal * 2) + 1500) {
        outputsSwitched = false;
        dprint("BONG");
      }
    }
  }
  else {
    p1OutputState = p2OutputState = p1OutputState | p2OutputState;
  }
  lastOutputsSwitched = outputsSwitched;
  writeToChip(p1OutputState, p2OutputState);
  dprint("OUTPUTS SWITCHED : ");
  dprint(outputsSwitched);
}

uint16_t spPlayerTakeover() {
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

uint16_t spBothControl() {
  /*
     This function returns any buttons pressed by either controller
  */
  uint16_t output;
  player1State = snes1.buttons();
  player2State = snes2.buttons();
  output = player1State | player2State;
  dprintState("BOTH CONTROL : ", player1State, player2State);
  writeToChip(output, output);
}

uint16_t spPlayersAgree() {
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
  writeToChip(output, output);
}

uint16_t spPlayersAlternate(uint16_t mTurnLength) {
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
  writeToChip(output, output);
}

// ==========================================================================
//                    Multi PLayer Modes of Operation
// ==========================================================================

void mpBothControl() {
  uint16_t alt_mask;
  alt_mask = playersAlternate(turnLength * 2);
  p1OutputState = snes1.buttons() | alt_mask;
  p2OutputState = snes2.buttons() | alt_mask;
  writeToChip(p1OutputState, p2OutputState);
  dprint("MP : ");
}

void mpPressedFirst() {
  temp1 = snes1.buttons();
      temp2 = snes2.buttons();
      p1OutputState = temp1 ^ temp2;
      p2OutputState = temp2 ^ temp1;
      writeToChip(p1OutputState, p2OutputState);
      dprintState("PRESSED FIRST :", p1OutputState, p2OutputState);
}

void mpLimitedPress() {
  p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();

      if (p1OutputState ^ p1LastOutputState != 0 && p1OutputState > p1LastOutputState) {
        p1Presses = p1Presses + 1;
        if (p1Presses > 15) {
          p1Does = false;
        }
        if (millis() > lastPressCycle + (potVal * 2 + 1200)) {
          lastPressCycle = millis();
          p1Does = true;
          p1Presses = 0;
        }
      }

      if (p2OutputState ^ p2LastOutputState != 0 && p2OutputState > p2LastOutputState) {
        p2Presses = p2Presses + 1;
        if (p2Presses > 15) {
          p2Does = false;
        }
        if (millis() > lastPressCycle + (potVal * 2  + 1200)) {
          lastPressCycle = millis();
          p2Does = true;
          p2Presses = 0;
        }
      }
      dprint(p1Does);
      dprint(p2Does);
      writeToChip(p1OutputState, p2OutputState);
      dprintln(" ");
      dprintState("LIMITED PRESS :", p1OutputState, p2OutputState);
}

void mpXOR() {
      p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      temp_mask = p1OutputState & p2OutputState;
      p1OutputState = p1OutputState ^ temp_mask;
      p2OutputState = p2OutputState ^ temp_mask;
      writeToChip(p1OutputState, p2OutputState);
      dprintState("MP XOR :", p1OutputState, p2OutputState);
}

void mpTakeControl() {
  p1OutputState = snes1.buttons();
      p2OutputState = snes2.buttons();
      dprint(outputsSwitched);
      dprintState(" MP TAKE CONTROL : ", p1OutputState, p2OutputState);
      if (p1OutputState & TRIGGER_MASK == TRIGGER_MASK) {
        //Serial.println(" ");
        dprintln(" ");
        if (millis() > p1LastTrigger + triggerDebouce) {
          p1LastTrigger = millis();
          lastOutputsSwitched = outputsSwitched;
          outputsSwitched = true;
        }
      }
      else if (p2OutputState & TRIGGER_MASK == TRIGGER_MASK) {
        //Serial.println(" ");
        dprintln(" ");
        if (millis() > p2LastTrigger + triggerDebouce) {
          p2LastTrigger = millis();
          lastOutputsSwitched = outputsSwitched;
          outputsSwitched = true;
        }
      }
      if (outputsSwitched == true) {
        tc_temp = p1OutputState;
        p1OutputState = p2OutputState;
        p2OutputState = tc_temp;
        if (p1LastTrigger > p2LastTrigger)


          //       outputsSwitched=
          //         millis() > (p1LastTrigger > p2LastTrigger ? p1LastTrigger : p2LastTrigger) + potVal * 2 + 800;


          if (millis() > p1LastTrigger + (potVal * 2) + 800) {
            outputsSwitched = false;
          }
          else {
            if (millis() > p2LastTrigger + (potVal * 2) + 800) {
              outputsSwitched = false;
            }
          }
      }
      writeToChip(p1OutputState, p2OutputState);
}

// ==========================================================================
//                Helper Functions for Modes of Operations
// ==========================================================================

void flashLeds() {
  STATUS_LEDS = p1OutputState;
}
