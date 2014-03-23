// LightwaveRF.cpp
//
// LightwaveRF 434MHz interface for Arduino
// 
// Author: Lawrie Griffiths (lawrie.griffiths@ntlworld.com)
// Copyright (C) 2013 Lawrie Griffiths

#include <LightwaveRF.h>

static byte lw_nibble[] = {0xF6,0xEE,0xED,0xEB,0xDE,0xDD,0xDB,0xBE,
                     0xBD,0xBB,0xB7,0x7E,0x7D,0x7B,0x77,0x6F};

static byte lw_rx_pin;
static byte lw_tx_pin;

static volatile boolean lw_got_message = false; // true when full message received
static const byte lw_msg_len = 10; // the expected length of the message
static byte lw_msg[lw_msg_len]; // the message received

static byte lw_byte; // The current byte
static byte lw_num_bits = 0; // number of bits in the current byte
static unsigned long lw_prev; // time of previous pulse
static volatile boolean lw_p_started = false; // packet started
static boolean lw_b_started = false; // byte started
static byte lw_num_bytes = 0; // number of bytes received 

static const int lw_bit_delay = 560;
static const byte lw_repeats = 12; // Number of repeats of message sent

static long lw_num_invalid_packets[4];

/**
  Pin change interrupt routine that identifies 1 and 0 LightwaveRF bits
  and constructs a message when a valid packet of data is received.
**/
void lw_process_bits() { 
  // Don't process bits when a message is already ready
  if (lw_got_message) return;
  
  byte v = digitalRead(lw_rx_pin); // the current value
  unsigned long curr = micros(); // the current time in microseconds
  
  // Calculate pulse duration in 50 microsecond units
  unsigned int dur = (curr-lw_prev)/50;
  lw_prev = curr;
  
  // See if pulse corresponds to expected bit length
  if (dur < 6) {
    // inter 1 bit gap - do nothing
  } else if (dur < 11) {
    // potential 1 bit
    if (!v) { // value now zero as 1 pulse ended
      // 1 bit
      if (!lw_p_started) {
        // Start of message
        lw_p_started = true;
        lw_b_started = false;
        lw_num_bytes = 0;
      } else if (!lw_b_started) {
        // byte started
        lw_b_started = true;
        lw_num_bits = 0;
        lw_byte = 0;
      } else {
        // a valid 1 bit
        lw_byte = (lw_byte << 1) | 1;
        if (++lw_num_bits == 8) { // Test for complete byte
		  // Add the byte to the message
		  lw_b_started = false;
		  lw_msg[lw_num_bytes++] = lw_byte;
        }
      }
    } else {
      // Too short for a zero bit
      lw_p_started = false;
	  if (lw_num_bytes > 0) lw_num_invalid_packets[0]++;
    }
  } else if (dur > 20 && dur < 28) {
    // potential 0 bit
    if (v) {
      // 0 bit
      if (!lw_b_started) {
        // Zero bit where byte start bit expected
        lw_p_started = false;
		if (lw_num_bytes > 0) lw_num_invalid_packets[1]++;
      } else if (lw_p_started) {
        // Valid 0 bit
        lw_byte = (lw_byte << 1);
        if (++lw_num_bits == 8) {
          // Add the byte to the message
          lw_msg[lw_num_bytes++] = lw_byte;
          lw_b_started = false;
        }
      }
    } else {
      // Too long for a 1 bit
      lw_p_started = false;
	  if (lw_num_bytes > 0) lw_num_invalid_packets[2]++;
    }
  } else {
     // Not a valid length for a bit
	 if (lw_p_started && lw_num_bytes > 0) lw_num_invalid_packets[3]++;
	 lw_p_started = false;
  }
  
  // See if we have the whole message
  if (lw_p_started && lw_num_bytes == lw_msg_len) {
    lw_got_message = true;
    lw_p_started = false;
  }
}

void lw_get_error_stats(long* inv) {
  for(int i=0;i<4;i++) inv[i] = lw_num_invalid_packets[i];
}


/**
  Wait for a message to arrive
**/
void lw_rx_wait()
{
    while (!lw_got_message);
}

/**
  Test if a message has arrived
**/
boolean lw_have_message()
{
    return lw_got_message;
}

/**
  Transfer a message to user buffer
**/
boolean lw_get_message(byte  *buf, byte *len) {
  if (!lw_got_message) return false;
  
  // Buffer length must be 10
  if (*len != lw_msg_len) return false;
  
  memcpy(buf,lw_msg,lw_msg_len);

  lw_got_message = false;
  return true;
}

/**
  Set things up to transmit LightwaveRF 434Mhz messages using the pin specified
**/
void lw_tx_setup(byte tx_pin) {
  lw_tx_pin = tx_pin;
  pinMode(lw_tx_pin,OUTPUT);
}

/**
  Set things up to receive LightwaveRF 434Mhz messages using the values specified
**/
void lw_rx_setup(byte rx_pin, byte interrupt) {
  lw_rx_pin = rx_pin;
  pinMode(lw_rx_pin,INPUT);
  attachInterrupt(interrupt,lw_process_bits,CHANGE);
}

/**
  Set things up to transmit and receive LightwaveRF 434Mhz messages using the values specified
**/
boolean lw_setup(byte tx_pin, byte rx_pin, byte interrupt) {
  if(tx_pin == rx_pin) {
    return false;
  }
  
  lw_tx_setup(tx_pin);
  lw_rx_setup(rx_pin, interrupt);
  
  return true;
}

/**
  Set things up to transmit and receive LightwaveRF 434Mhz messages using default values
**/
void lw_setup() {
  lw_setup(3, 2, 0);
}

/**
Transmit a 1 or 0 bit to a LightwaveRF device
**/
void lw_send_bit(byte b) {
  delayMicroseconds(25);
  digitalWrite(lw_tx_pin,b);
  delayMicroseconds(lw_bit_delay/2);
  digitalWrite(lw_tx_pin,LOW);
  delayMicroseconds(lw_bit_delay/2);
  
  if (b == LOW) {
    delayMicroseconds(300);
  }
}

/**
  Send a LightwaveRF byte
**/
void lw_tx_byte(byte b) {
  lw_send_bit(HIGH);

  for (byte mask = B10000000; mask; mask >>= 1) {
    lw_send_bit(b & mask);
  }
}

/**
  Send a LightwaveRF message
**/
void lw_send(byte* msg) {
  cli();
  for(byte j=0;j<lw_repeats;j++) {
    // send a start bit
    lw_send_bit(HIGH);
    
	// Send the 10 bytes
    for(byte i=0;i<lw_msg_len;i++) lw_tx_byte(msg[i]);
    
    // send end bit
    lw_send_bit(HIGH);
	
	// Delay between repeats
    delayMicroseconds(10250);
  } 
  sei();  
}

/**
  Send a LightwaveRF command
**/
void lw_cmd(byte level, byte channel, byte cmd, byte* id) {
  byte msg[10];
  
  msg[0] = lw_nibble[level >> 4];
  msg[1] = lw_nibble[level & 0xF];
  msg[2] = lw_nibble[channel];
  msg[3] = lw_nibble[cmd];
  
  for(byte i=0;i<6;i++) {
    msg[4+i] = id[i];
  }
  lw_send(msg);
}
