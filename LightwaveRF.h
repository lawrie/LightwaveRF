// LightwaveRF.h
//
// LightwaveRF 434MHz interface for Arduino
//
// Author: Lawrie Griffiths (lawrie.griffiths@ntlworld.com)
// Copyright (C) 2012 Lawrie Griffiths

#include <Arduino.h>

#define NUM_DIM_LEVELS  31U


typedef enum LWRF_COMMANDS_ENUM
{
    LWRF_COMMAND_OFF = 0xF6,
    LWRF_COMMAND_ON  = 0xEE,
    LWRF_MOOD_0      = 0xBD,
    LWRF_MOOD_1      = 0xBB,
    LWRF_MOOD_2      = 0xB7,
    LWRF_MOOD_3      = 0x7E,
    LWRF_MOOD_4      = 0x6F
}  LWRFCommandsEnum ;

typedef enum LWRF_DIM_LEVELS
{
    DIM_LEVEL_0  = 0xBDF6,
    DIM_LEVEL_1  = 0xBDEE,
    DIM_LEVEL_2  = 0xBDED,
    DIM_LEVEL_3  = 0xBDEB,
    DIM_LEVEL_4  = 0xBDDE,
    DIM_LEVEL_5  = 0xBDDD,
    DIM_LEVEL_6  = 0xBDDB,
    DIM_LEVEL_7  = 0xBDBE,
    DIM_LEVEL_8  = 0xBDBD,
    DIM_LEVEL_9  = 0xBDBB,
    DIM_LEVEL_10 = 0xBDB7,
    DIM_LEVEL_11 = 0xBD7E,
    DIM_LEVEL_12 = 0xBD7D,
    DIM_LEVEL_13 = 0xBD7B,
    DIM_LEVEL_14 = 0xBD77,
    DIM_LEVEL_15 = 0xBD6F,
    DIM_LEVEL_16 = 0xBBF6,
    DIM_LEVEL_17 = 0xBBEE,
    DIM_LEVEL_18 = 0xBBED,
    DIM_LEVEL_19 = 0xBBEB,
    DIM_LEVEL_20 = 0xBBDE,
    DIM_LEVEL_21 = 0xBBDD,
    DIM_LEVEL_22 = 0xBBDB,
    DIM_LEVEL_23 = 0xBBBE,
    DIM_LEVEL_24 = 0xBBBD,
    DIM_LEVEL_25 = 0xBBBB,
    DIM_LEVEL_26 = 0xBBB7,
    DIM_LEVEL_27 = 0xBB7E,
    DIM_LEVEL_28 = 0xBB7D,
    DIM_LEVEL_29 = 0xBB7B,
    DIM_LEVEL_30 = 0xBB77,
    DIM_LEVEL_31 = 0XBB6F
}LWRFDimEnum;




typedef struct LWRF_STATE_CODE
{

    uint8_t HighB;
    uint8_t LowB;
} LWRFStateCodeStruct;

typedef struct LWRF_REMOTE_ID
{
    uint8_t Byte5;
    uint8_t Byte4;
    uint8_t Byte3;
    uint8_t Byte2;
    uint8_t Byte1;
    uint8_t Byte0;
} LWRFRemoteIDStruct;

typedef struct LWRF_MESSAGE_STRUCT
{
    union
    {
        LWRFStateCodeStruct StateCodeBytes;
        LWRFDimEnum DimLevel;
    };
    uint8_t SwitchID;
    uint8_t FunctionByte;
    LWRFRemoteIDStruct RemoteID;
} LWRFMessageStruct;



typedef struct LWRF_PAIRING_STRUCT
{ 
    LWRFRemoteIDStruct RemoteID;
    uint8_t SwitchID;
    bool Paired;
} LWRFPairingStruct;

typedef struct LWRF_EEPROM_HEADER_STRUCT
{
    uint8_t NumPairings;
    LWRF_PAIRING_STRUCT PairedRemotes[6];
} LWRFEepromHeaderStruct;

extern uint16_t DimLevels[];

extern void lw_setup();

extern void lw_print_Msg( LWRFMessageStruct* Msg, int LineNUm);

extern void lw_rx_wait();

extern boolean lw_have_message();

extern boolean lw_get_message(byte* buf, byte* len);
extern boolean lw_get_msg(LWRFMessageStruct* msg);
extern void lw_pair_device(void);
extern void lw_recall_pairing(void);
extern void lw_send(byte* msg);
extern void lw_clear_pairings();
extern boolean lw_search_pairing(LWRFMessageStruct* msg);
