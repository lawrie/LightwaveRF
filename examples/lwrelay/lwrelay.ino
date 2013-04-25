// Binary data relay of data between PC and Arduino
// All ten bytes LightwaveRF received from the transceiver
// are sent to the PC and all ten byte packets from the PC
// are transmitted.
// 
// Author Lawrie Griffiths

#include <LightwaveRF.h>

void setup() {
  lw_setup();
  Serial.begin(57600);
}

void loop() {
  byte msg[10];
  byte len = 10;

  if (lw_have_message()) {
    lw_get_message(msg,&len);
    for(int i=0;i<len;i++)  Serial.write(msg[i]);
  }
  
  if (Serial.available() >= 10) {
    for(int i=0;i<10;i++) {
      msg[i] = Serial.read();
      Serial.write(msg[i]);
    }
    lw_send(msg);
  }
}
