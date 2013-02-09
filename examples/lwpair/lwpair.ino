
/******************************************************************************
lwpair.ino

Arduino Example that will pair, and respond to paired LightWave RF remotes.
Author: Jeffrey Marten Gillmor
Copyright (C) 2013 Jeffrey Marten Gillmor
*******************************************************************************/

#include <EEPROM.h>
#include <LightwaveRF.h>

/* Uncomment this to print all messages that are received */
//PRINT_ALL_MESSAGES

#define FULL_BRIGHTNESS 0xFFU
#define BRIGHTNESS_MULTIPLIER 8U

long debounceDelay = 50;                    // the debounce time; increase if the output flickers
const int pairingInputPin = 4;              // the number of the pushbutton pin used to pair remotes
const int ledPin =  13;                     // the number of the LED pin to indicate pairing status
const int clearPin = 5;                     // the number of the pushbutton pin used unpair all remotes.
const int outputPin = 9;                     // the number of the pin that is connected to the light that LWRF controls

int ledState = HIGH;                        // the current state of the output pin
int pairingInputPinState;                   // the current reading from the pairing pin
int clearState;                             // the current reading from the  clear pairings input pin
static int lastPairingInputPinState = LOW;  // the previous reading from the pairing input pin
static int lastClearState = LOW;            // the previous reading from the pairing sinput pin


static void checkPairRequest(void );
static void checkClearRequest(void);



/******************************************************************************
setup

Setup the hardware, and recall all previously stored pairings
*******************************************************************************/
void setup()
{
    pinMode(pairingInputPin, INPUT);
    pinMode(clearPin, INPUT);
    pinMode(ledPin, OUTPUT);
    pinMode(outputPin, OUTPUT);


    Serial.begin(57600);

    lw_setup();
    lw_recall_pairing();
}


/******************************************************************************
loop

Main arduino loop
*******************************************************************************/
void loop()
{
    uint8_t DimIDX = 0U;
    /* Create a local structure to store the last recevied message */
    LWRFMessageStruct RxMsg;

    /* If message has been received */
    if (lw_get_msg(&RxMsg) == true)
    {

#ifdef PRINT_ALL_MESSAGES
        lw_print_Msg(&RxMsg, __LINE__);
#endif
        /* Check if this arduino has been paired with the message */
        if (lw_search_pairing(&RxMsg))
        {
            Serial.println(F("Found a Paired message"));

            /* Check the message */

            if (RxMsg.FunctionByte == (uint8_t)LWRF_COMMAND_ON)
            {
                analogWrite(outputPin, FULL_BRIGHTNESS);
            }
            else if (RxMsg.FunctionByte == (uint8_t)LWRF_COMMAND_OFF)
            {
                analogWrite(outputPin, 0U);
            }
            else
            {
                DimIDX = 0U;
                while (DimIDX < NUM_DIM_LEVELS)
                {
                    if (DimLevels[DimIDX] == RxMsg.FunctionByte )
                    {
                        analogWrite(outputPin, DimIDX * BRIGHTNESS_MULTIPLIER);
                    }
                    DimIDX++;
                }
            }
        }
        else
        {
            Serial.println(F("Ignored an unpaired message"));
        }
    }

    /* Process all the debounced inputs */
    checkPairRequest();
    checkClearRequest();

}


/******************************************************************************
checkPairRequest

This debounces the pair request button, and if a paring has been requested, asks
the LightwaveRF to pair with the next received message.
*******************************************************************************/
static void checkPairRequest(void)
{
    static bool startPair = false;
    static long lastDebounceTime = 0;  /* the last time the output pin was toggled */

    /* read the state of the switch into a local variable: */
    int reading = digitalRead(pairingInputPin);

    /* check to see if you just pressed the button
       (i.e. the input went from LOW to HIGH),  and you've waited
       long enough since the last press to ignore any noise:  */

    /* If the switch changed, due to noise or pressing: */
    if (reading != lastPairingInputPinState)
    {
        /* reset the debouncing timer */
        lastDebounceTime = millis();
        startPair = true;
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        /* whatever the reading is at, it's been there for longer
           than the debounce delay, so take it as the actual current state: */
        pairingInputPinState = reading;
        digitalWrite(ledPin, pairingInputPinState);
        if ((pairingInputPinState) && (startPair))
        {
            lw_pair_device();
            startPair = false;
        }
    }

    /* save the reading.  Next time through the loop,
       it'll be the lastPairingInputPinState: */
    lastPairingInputPinState = reading;
}


/******************************************************************************
checkPairRequest

This debounces the clear all pairings button and if a clear has been requested,
it asks the LightwaveRF to remove all parings from the EEPROM
*******************************************************************************/
static void checkClearRequest(void)
{

    static bool startClear = false;
    static long lastDebounceTime = 0;

    /* read the state of the switch into a local variable: */
    int reading = digitalRead(clearPin);

    /*check to see if you just pressed the button
      (i.e. the input went from LOW to HIGH),  and you've waited
      long enough since the last press to ignore any noise: */

    /* If the switch changed, due to noise or pressing: */
    if (reading != lastClearState)
    {
        /* reset the debouncing timer */
        lastDebounceTime = millis();
        startClear = true;
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        /* whatever the reading is at, it's been there for longer
         than the debounce delay, so take it as the actual current state: */
        clearState = reading;

        if ((clearState) && (startClear))
        {
            lw_clear_pairings();
            startClear = false;
        }
    }

    /* save the reading.  Next time through the loop,
       it'll be the lastPairingInputPinState: */
    lastClearState = reading;
}
