#include <LightwaveRF.h>

static byte id[] = {0xF6,0xF6,0xBD,0x77,0x77,0xED};

void setup() {
  lw_setup();
  Serial.begin(57600);
}

void loop() {
  if (Serial.available()) {
    byte c = Serial.read();
    
    switch (c) {
      case 'l':
        Serial.println("Locking device");
        lw_cmd(128,0,LW_LOCK, id); // Lock the device
        break;
      case 'u':
        Serial.println("Unlocking device");
        lw_cmd(0,0,LW_LOCK,id); // Unlock the device
        break;
      case 'a':
        Serial.println("Locking all");
        lw_cmd(143,0,LW_LOCK,id); // Lock all for the device
        break;
      default:
         Serial.println("Invalid command");
         break;
    }
  }
}



