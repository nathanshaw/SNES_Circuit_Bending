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
    else if (mode == MP_LIMITED_PRESS) {
      PLAYER1_BUTTONS1 = ~(byte)(player1 * p1Does);
      PLAYER1_BUTTONS2 = ~(((player1 * p1Does) >> 8) | 0x00);
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
    else if (mode == MP_LIMITED_PRESS) {
      PLAYER2_BUTTONS1 = ~(byte)(player2 * p2Does);
      PLAYER2_BUTTONS2 = ~(((player2 * p2Does) >> 8) | 0x00);
    }
    else {
      PLAYER2_BUTTONS1 = ~(byte)player2;
      PLAYER2_BUTTONS2 = ~((player2 >> 8) | 0x00);
    }
    p2LastOutputState = player2;
  }
}

