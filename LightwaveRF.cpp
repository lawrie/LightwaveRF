// LightwaveRF.cpp
//
// LightwaveRF 434MHz interface for Arduino
//
// Author: Lawrie Griffiths (lawrie.griffiths@ntlworld.com)
// Copyright (C) 2012 Lawrie Griffiths

#include <LightwaveRF.h>
#include <../../../../libraries/EEPROM/EEPROM.h>
#include <Arduino.h>


#define NUM_PAIRING_OFFSET 0U
#define PAIRING_TIMEOUT_MS 30000U
#define MAX_PAIRINGS    6U
#define FIRST_PAIRING_OFFSET 1U
#define PAIR_EEPROM_START(x)  (FIRST_PAIRING_OFFSET + (x * sizeof(LWRF_PAIRING_STRUCT)))
#define PAIR_EEPROM_END(x)  (sizeof(LWRF_PAIRING_STRUCT) + FIRST_PAIRING_OFFSET + (x * sizeof(LWRF_PAIRING_STRUCT)))


static int lw_rx_pin = 2;
static int lw_tx_pin = 3;


uint16_t DimLevels[] = 
{
    DIM_LEVEL_0 ,
    DIM_LEVEL_1 ,
    DIM_LEVEL_2 ,
    DIM_LEVEL_3 ,
    DIM_LEVEL_4 ,
    DIM_LEVEL_5 ,
    DIM_LEVEL_6 ,
    DIM_LEVEL_7 ,
    DIM_LEVEL_8 ,
    DIM_LEVEL_9 ,
    DIM_LEVEL_10,
    DIM_LEVEL_11,
    DIM_LEVEL_12,
    DIM_LEVEL_13,
    DIM_LEVEL_14,
    DIM_LEVEL_15,
    DIM_LEVEL_16,
    DIM_LEVEL_17,
    DIM_LEVEL_18,
    DIM_LEVEL_19,
    DIM_LEVEL_20,
    DIM_LEVEL_21,
    DIM_LEVEL_22,
    DIM_LEVEL_23,
    DIM_LEVEL_24,
    DIM_LEVEL_25,
    DIM_LEVEL_26,
    DIM_LEVEL_27,
    DIM_LEVEL_28,
    DIM_LEVEL_29,
    DIM_LEVEL_30,
    DIM_LEVEL_31
};

/* Pairing info */
static uint8_t NumPairedRemotes = 0;
static LWRFEepromHeaderStruct LWRFEeprom;

static volatile boolean lw_got_message = false; // true when full message received
static const byte lw_msg_len = 10; // the expected length of the message
static byte lw_msg[lw_msg_len]; // the message received

static byte lw_byte; // The current byte
static byte lw_num_bits = 0; // number of bits in the current byte
static unsigned long lw_prev; // time of previous pulse
static boolean lw_p_started = false; // packet started
static boolean lw_b_started = false; // byte started
static byte lw_num_bytes = 0; // number of bytes received

static const int lw_bit_delay = 640;

static const byte lw_repeats = 12; // Number of repeats of message sent

/**
  Pin change interrupt routine that identifies 1 and 0 LightwaveRF bits
  and constructs a message when a valid packet of data is received.
**/
void lw_process_bits()
{
    if (lw_got_message) return;

    byte v = digitalRead(lw_rx_pin); // the current value
    unsigned long curr = micros(); // the current time in microseconds

    // Calculate pulse duration in 50 microsecond units
    unsigned int dur = (curr - lw_prev) / 50;
    lw_prev = curr;

    // See if pulse corresponds to expected bit length
    if (dur < 6)
    {
        // inter 1 bit gap - do nothing
    }
    else if (dur < 11)
    {
        // potential 1 bit
        if (!v)   // value now zero as 1 pulse ended
        {
            // 1 bit
            if (!lw_p_started)
            {
                // Start of message
                lw_p_started = true;
                lw_b_started = false;
                lw_num_bytes = 0;
            }
            else if (!lw_b_started)
            {
                // byte started
                lw_b_started = true;
                lw_num_bits = 0;
                lw_byte = 0;
            }
            else
            {
                // a valid 1 bit
                lw_byte = (lw_byte << 1) | 1;
                if (++lw_num_bits == 8)   // Test for complete byte
                {
                    if (lw_num_bytes == 0 && lw_byte != 0xf6)   // Check first byte of message
                    {
                        // Does not start with f6, could have missed first bit
                        if (lw_byte == 0xdb)
                        {
                            // simulate getting the correct f6 byte
                            lw_b_started = true;
                            lw_msg[lw_num_bytes++] = 0xf6;
                            lw_byte = 0;
                            lw_num_bits = 0;
                        }
                        else
                        {
                            // not a valid packet after all
                            lw_p_started = false;
                            lw_num_bytes = 0;
                            lw_got_message = false;
                        }
                    }
                    else
                    {
                        // Add the byte to the message
                        lw_b_started = false;
                        lw_msg[lw_num_bytes++] = lw_byte;
                    }
                }
            }
        }
        else
        {
            // Too short for a zero bit
            lw_p_started = false;
            lw_num_bytes = 0;
            lw_got_message = false;
        }
    }
    else if (dur > 20 && dur < 28)
    {
        // potential 0 bit
        if (v)
        {
            // 0 bit
            if (!lw_b_started)
            {
                // Zero bit where byte start bit expected
                lw_p_started = false;
                lw_num_bytes = 0;
                lw_got_message = false;
            }
            else if (lw_p_started)
            {
                // Valid 0 bitnumB
                lw_byte = (lw_byte << 1);
                if (++lw_num_bits == 8)
                {
                    if (lw_num_bytes == 0 && lw_byte != 0xf6)
                    {
                        // Not a valid message after all
                        lw_p_started = false;
                        lw_num_bytes = 0;
                        lw_got_message = false;
                    }
                    else
                    {
                        // Add the byte to the message
                        lw_msg[lw_num_bytes++] = lw_byte;
                        lw_b_started = false;
                    }
                }
            }
        }
        else
        {
            // Too long for a 1 bit
            lw_p_started = false;
            lw_num_bytes = 0;
            lw_got_message = false;
        }
    }
    else
    {
        // Not a valid length for a bit
        lw_p_started = false;
        lw_num_bytes = 0;
        lw_got_message = false;
    }

    // See if we have the whole message
    if (lw_num_bytes == lw_msg_len)
    {
        lw_got_message = true;
        lw_p_started = false;
    }
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
boolean lw_get_message(byte  *buf, byte *len)
{
    if (!lw_got_message) return false;

    byte rxLen = lw_msg_len;
    if (*len < rxLen) *len = rxLen;

    memcpy(buf, lw_msg, *len);
    memset(lw_msg, 0, rxLen);
    lw_got_message = false;
    return true;
}


boolean lw_get_msg(LWRFMessageStruct *msg)
{
    boolean pairingFound = false;
    if (!lw_got_message)
    {
        return false;
    }
    else
    {
        lw_got_message = false;
        pairingFound = true;

        memcpy(msg, lw_msg, sizeof(LWRFMessageStruct));
        memset(lw_msg, 0, sizeof(lw_msg));

    }
    return true;
}

boolean lw_search_pairing(LWRFMessageStruct *msg)
{
    boolean pairingFound = false;
    uint8_t pairIdx = 0U;
    uint8_t byteIdx = 0U;
    uint8_t *pairMsgPtr;
    uint8_t *rxMsgPtr;

    while((pairIdx < NumPairedRemotes ) && (pairingFound == false))
    {

        pairMsgPtr = &LWRFEeprom.PairedRemotes[pairIdx].RemoteID.Byte5;
        rxMsgPtr = &msg->RemoteID.Byte5;
        byteIdx = 0U;

        /* Think positive, set the pairingFound to true , until the two
           messages no longer match. */
        pairingFound = true;

        while(byteIdx < sizeof(LWRFRemoteIDStruct))
        {
            if(*rxMsgPtr++ != *pairMsgPtr++)
            {
                pairingFound = false;
            } 
            byteIdx++;
        }
        if (msg->SwitchID != LWRFEeprom.PairedRemotes[pairIdx].SwitchID)
        {
            pairingFound = false;
        }
        if (!LWRFEeprom.PairedRemotes[pairIdx].Paired)
        {
            pairingFound = false;
        }
        pairIdx++;
    }
    return pairingFound;
}

/**
  Set things up to receive and transmit LighWaveRF 434Mhz messages
**/
void lw_setup()
{
    pinMode(lw_rx_pin, INPUT);
    pinMode(lw_tx_pin, OUTPUT);
    attachInterrupt(0, lw_process_bits, CHANGE);

    lw_recall_pairing();

}

/**
  Transmit a 1 or 0 bit to a LightwaveRF device
**/
void lw_send_bit(byte b)
{
    delayMicroseconds(25);
    digitalWrite(lw_tx_pin, b);
    delayMicroseconds(lw_bit_delay / 2);
    digitalWrite(lw_tx_pin, LOW);
    delayMicroseconds(lw_bit_delay / 2);
}

/**
  Send a LightwaveRF byte
**/
void lw_tx_byte(byte b)
{
    lw_send_bit(HIGH);

    for (byte mask = B10000000; mask; mask >>= 1)
    {
        //Serial.println(b & mask);
        lw_send_bit(b & mask);
    }
}

/**
  Send a LightwaveRF message
**/
void lw_send(byte *msg)
{
    cli();
    for (byte j = 0; j < lw_repeats; j++)
    {
        // send a start bit
        lw_send_bit(HIGH);

        // Send the 10 bytes
        for (byte i = 0; i < lw_msg_len; i++) lw_tx_byte(msg[i]);

        // send end bit
        lw_send_bit(HIGH);
        delayMicroseconds(10000);
    }
    sei();
}

void lw_clear_pairings()
{
    Serial.print(F("\r\nNow clearing "));
    Serial.print(NumPairedRemotes);
    Serial.println(F(" parings" ));
    for ( uint16_t EepromAddr = 0;  EepromAddr < sizeof(LWRFEepromHeaderStruct); EepromAddr++)
    {
        EEPROM.write(EepromAddr,0);
    }
    Serial.println(F("All pairings erased"));
    lw_recall_pairing();
}


void lw_pair_device(void)
{
    LWRFMessageStruct PairMsg;
    LWRFPairingStruct EepromPair;
    uint32_t StartTime = millis();
    uint16_t EepromAddr = 0U;
    uint8_t *EepromBytePtr = (uint8_t *)&EepromPair;


    memset((void *)&PairMsg, 0 , sizeof(PairMsg));

    Serial.print(F("\r\nWaiting for a message to pair with T:"));
    Serial.println(millis());
    /* Wait for a remote message */
    while ((!lw_got_message) && ((millis() - StartTime) <= PAIRING_TIMEOUT_MS));

    if (!lw_got_message)
    {
        Serial.print(F("No pairing found T:"));
        Serial.println(millis());
    }
    else
    {
        Serial.print(F("Message found for pairing T:"));
        Serial.println(millis());
        lw_get_msg(&PairMsg);
        lw_print_Msg(&PairMsg, __LINE__);
        lw_got_message = false;

        if (NumPairedRemotes < MAX_PAIRINGS )
        {
            Serial.println(F("Space Available for new remote"));
            

            EepromPair.Paired = true;
            memcpy((void *)&EepromPair.RemoteID, (void *)&PairMsg.RemoteID, sizeof(LWRFRemoteIDStruct));
            EepromPair.SwitchID = PairMsg.SwitchID;
       

            for (EepromAddr = PAIR_EEPROM_START(NumPairedRemotes);  EepromAddr < PAIR_EEPROM_END(NumPairedRemotes) ; EepromAddr++)
            {
                EEPROM.write(EepromAddr, *EepromBytePtr);
                EepromBytePtr++;
            }

            /* Update the number of parings */
            NumPairedRemotes++;
            EEPROM.write(NUM_PAIRING_OFFSET, NumPairedRemotes);
           
            lw_recall_pairing();
            
        }

    }
}



void lw_print_Msg( LWRFMessageStruct *Msg, int LineNUm)
{
    Serial.print(LineNUm);
    Serial.println(F("\r\nFound a Message:"));
    Serial.print(F("  State code: 0x"));
    Serial.println(Msg->DimLevel, HEX);
    Serial.print(F("  Switch ID: 0x"));
    Serial.println(Msg->SwitchID, HEX);
    Serial.print(F("  Function: 0x"));
    Serial.println(Msg->FunctionByte, HEX);
    Serial.print(F("  Remote ID: 0x"));
    Serial.print(Msg->RemoteID.Byte5, HEX);
    Serial.print(F(" 0x"));
    Serial.print(Msg->RemoteID.Byte4, HEX);
    Serial.print(F(" 0x"));
    Serial.print(Msg->RemoteID.Byte3, HEX);
    Serial.print(F(" 0x"));
    Serial.print(Msg->RemoteID.Byte2, HEX);
    Serial.print(F(" 0x"));
    Serial.print(Msg->RemoteID.Byte1, HEX);
    Serial.print(F(" 0x"));
    Serial.println(Msg->RemoteID.Byte0, HEX);
    Serial.println(F(" "));
}

void lw_recall_pairing(void)
{
    uint16_t EepromAddr = 0U;
    uint8_t *EepromBytePtr = (uint8_t *)&LWRFEeprom;
    uint8_t pairingIdx = 0U;

    Serial.println(F("\r\n\r\nRecalling pairing:"));
    for (EepromAddr = NUM_PAIRING_OFFSET; EepromAddr < sizeof(LWRFEepromHeaderStruct); EepromAddr++)
    {

        *EepromBytePtr = EEPROM.read(EepromAddr);
         Serial.print(F(" 0x"));
         Serial.print(*EepromBytePtr , HEX);
        EepromBytePtr++;
    }
    Serial.print(F(" Num Remotes: "));
    Serial.println(LWRFEeprom.NumPairings);
    NumPairedRemotes = LWRFEeprom.NumPairings;
    for(pairingIdx = 0U; pairingIdx < LWRFEeprom.NumPairings; pairingIdx++)
    {
        Serial.print(F(" Remote["));
        Serial.print(pairingIdx);
        Serial.println(F("]"));
        Serial.print(F(" \tID: 0x"));
        Serial.print(LWRFEeprom.PairedRemotes[pairingIdx].RemoteID.Byte5, HEX);
        Serial.print(F(" 0x"));
        Serial.print(LWRFEeprom.PairedRemotes[pairingIdx].RemoteID.Byte4, HEX);
        Serial.print(F(" 0x"));
        Serial.print(LWRFEeprom.PairedRemotes[pairingIdx].RemoteID.Byte3, HEX);
        Serial.print(F(" 0x"));
        Serial.print(LWRFEeprom.PairedRemotes[pairingIdx].RemoteID.Byte2, HEX);
        Serial.print(F(" 0x"));
        Serial.print(LWRFEeprom.PairedRemotes[pairingIdx].RemoteID.Byte1, HEX);
        Serial.print(F(" 0x"));
        Serial.println(LWRFEeprom.PairedRemotes[pairingIdx].RemoteID.Byte0, HEX);
        Serial.print(F("\tSwitchID: 0x"));
        Serial.println(LWRFEeprom.PairedRemotes[pairingIdx].SwitchID,HEX);
    }

}

