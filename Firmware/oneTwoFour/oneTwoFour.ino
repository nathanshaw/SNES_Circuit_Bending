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
    PLAYER 1 : INPUT    (
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

#define IN1_LATCH 3
#define IN1_CLOCK 2
#define IN1_DATA 5
#define IN2_LATCH 7
#define IN2_CLOCK 6
#define IN2_DATA 8
#define POT_SIG A15

// --------------------- PORTS -----------------------------

#define PLAYER1_BUTTONS1 PORTA
#define PLAYER1_BUTTONS2 PORTC
#define PLAYER2_BUTTONS1 PORTL
#define PLAYER2_BUTTONS2 PORTK

// --------------------- Controlers and Players -------------------
// ================================================================
//                            Globals
// ================================================================

uint8_t DEBUG = 5;

uint16_t p1OutputState;
uint16_t p2OutputState;


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
  // DDRF = 0xFF;
  // set controller ports to HIGH (the resting state)
  PLAYER1_BUTTONS1 = 0xFF;
  PLAYER1_BUTTONS2 = 0xFF;
  PLAYER2_BUTTONS1 = 0xFF;
  PLAYER2_BUTTONS2 = 0xFF;
  // set LED's port to LOW
  // STATUS_LEDS = 0x00;
  // let things settle
  delay(10);
  if (DEBUG) {
    Serial.begin(57600);
    Serial.println("SERIAL BUS OPENED");
  }
}

void loop() {
  p1OutputState = snes1.buttons();
  p2OutputState = snes2.buttons();
  writeToChip(p1OutputState, p2OutputState);
  if (DEBUG) {
    delay(DEBUG*10);
    printBits(p1OutputState);
    Serial.print("-|-");
    printBits(p2OutputState);
    Serial.println(" ");
    
  }
}

// ==========================================================================
//                       Communicating with the SNES('s)
// ==========================================================================

void writeToChip(uint16_t data1, uint16_t data2) {
  PLAYER1_BUTTONS1 = 0xFF;
  PLAYER1_BUTTONS2 = 0xFF;
  PLAYER2_BUTTONS1 = 0xFF;
  PLAYER2_BUTTONS2 = 0xFF;
    PLAYER1_BUTTONS1 = ~(byte)data1;
    PLAYER1_BUTTONS2 = ~((data1 >> 8) | 0x00);
    PLAYER2_BUTTONS1 = ~(byte)data2;
    PLAYER2_BUTTONS2 = ~((data2 >> 8) | 0x00);
}

// ============================================================
// ------------ Printing and DeBugging ----------------
// ============================================================

void printBits(uint16_t myByte) {
  for (uint16_t mask = 0x8000; mask; mask >>= 1) {
    if (mask & myByte)
      Serial.print('1');
    else
      Serial.print('0');
  }
}
