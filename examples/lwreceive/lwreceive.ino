#include <LightwaveRF.h>

void setup() {
  lw_setup();
  Serial.begin(57600);
}

void loop() {
  byte msg[10];
  byte len = 10;
  
  lw_rx_wait();
  lw_get_message(msg,&len);
  printMsg(msg, len);
}

void printMsg(byte *msg, byte len) {
  for(int i=0;i<len;i++) {
    Serial.print(msg[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
}
