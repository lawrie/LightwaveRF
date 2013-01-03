// LightwaveRF.h
//
// LightwaveRF 434MHz interface for Arduino
// 
// Author: Lawrie Griffiths (lawrie.griffiths@ntlworld.com)
// Copyright (C) 2012 Lawrie Griffiths

#include <Arduino.h>

extern void lw_setup();

extern void lw_rx_wait();

extern boolean lw_have_message();

extern boolean lw_get_message(byte* buf, byte* len);

extern void lw_send(byte* msg);

