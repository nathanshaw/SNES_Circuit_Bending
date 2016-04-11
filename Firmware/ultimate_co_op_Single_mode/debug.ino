// ============================================================
// ------------ Printing and DeBugging ----------------
// ============================================================

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
  if (mode == MP_LIMITED_PRESS) {
    dprint(" LP OUTPUTS : ");
    printBits(p1OutputState * p1Does);
    dprint(" : ");
    printBits(p2OutputState * p2Does);
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

