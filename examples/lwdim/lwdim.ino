#include <LightwaveRF.h>
                     
byte myid[] = {0x6F,0xEB,0xBE,0xED,0xB7,0x7B};

void setup() {
  lw_setup();
  Serial.begin(57600);
}

int level = 0;

void loop() {
  byte msg[10];
  byte len = 10;

  Serial.print("Setting level to ");
  Serial.println(level);
  lw_cmd(0x80 + level,6,LW_ON,myid);
  delay(10000);
  level++;
  if (level > 31) level = 0;
}
