void printStats() {
  if (DEBUG) {
    if (millis() > PRINT_INTERVAL + last_print) {
      Serial.print("INS: ");
      for (int i = 0; i < 4; i++) {
        if (input_value[i] < 100) {
          Serial.print(" ");

          if (input_value[i] < 10 ) {
            Serial.print(" ");
          }
        }
        Serial.print(input_value[i]);
        Serial.print(" ");
      }

      Serial.print("| ");
      Serial.print("POTS:");
      //print pots
      for (int i = 0; i < 4; i++) {
        if (pot_value[i] < 1000) {
          Serial.print(" ");
          if (pot_value[i] < 100) {
            Serial.print(" ");
            if (pot_value[i] < 10) {
              Serial.print(" ");
            }
          }
        }
        Serial.print(pot_value[i]);
        Serial.print(" ");
      }
      Serial.print("| ");
      Serial.print("D-OUTS : ");
      for (int i = 0; i < 4; i++) {
        Serial.print(digital_value[i]);
        Serial.print(" ");
      }
      Serial.print("| ");
      Serial.print("PWM_O:");
      for (int i = 0; i < 4; i++) {
        if (pwm_value[i] < 100) {
          Serial.print(" ");
          if (pwm_value[i] < 10) {
            Serial.print(" ");
          }
        }

        Serial.print(pwm_value[i]);
        Serial.print(" ");
      }
      Serial.print("| ");
      Serial.print("SCALERS:");
      for (int i = 0; i < 4; i++) {
        Serial.print(digital_delay_high[i]);
        Serial.print("-");
        Serial.print(digital_delay_low[i]);
        Serial.print(" ");
      }
      Serial.println();
      last_print = millis();
    }
  }
}

