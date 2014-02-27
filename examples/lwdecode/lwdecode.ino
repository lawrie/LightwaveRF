#include <LightwaveRF.h>


static byte nibbles[] = {0xF6,0xEE,0xED,0xEB,0xDE,0xDD,0xDB,0xBE,
                     0xBD,0xBB,0xB7,0x7E,0x7D,0x7B,0x77,0x6F};
                     
void setup() {
  lw_setup();
  Serial.begin(57600);
}

void loop() {
  byte msg[10];
  byte len = 10;
  
  lw_rx_wait();
  lw_get_message(msg,&len);
  decodeMsg(msg, len);
}

void decodeMsg(byte *msg, byte len) {
  byte level1 = findNibble(msg[0]);
  byte level2 = findNibble(msg[1]); 
  Serial.print("Level = ");
  Serial.println((level1 << 4) + level2);
  Serial.print("Channel = ");
  Serial.println(findNibble(msg[2]));
  Serial.print("Command = ");
  byte cmd = findNibble(msg[3]);
  Serial.println((cmd == 0 ? "Off" : (cmd == 1 ? " On" : (cmd == 2 ? "Mood" : String(cmd)))));
  Serial.print("Id = ");
  
  for(int i=0;i<6;i++) {
    Serial.print(msg[i+4],HEX);
    Serial.print(" ");
  }
  Serial.println();
}

int findNibble(byte b) {
  for(int i=0;i<16;i++) {
    if (b == nibbles[i]) return i;
  }
  return -1;
}
