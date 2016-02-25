
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

