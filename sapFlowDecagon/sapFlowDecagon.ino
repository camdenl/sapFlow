/*

MOTEINO SKETCH TO READ 4 SAP FLOW VALUES ONCE PER MINUTE,
AVERAGE THESE 4 VALUES,
TAKE MEDIAN OF VALUES OVER 15 MINUTES,
THEN SEND MEDIAN VALUE TO GATEWAY

*/




///LIBRARIES
#include <RFM69.h>
#include <SPI.h>
#include <Wire.h>
#include <MCP342x.h>
#include <LowPower.h>
#include "FastRunningMedian.h"
#include "DecaDuino.h" 

//DEFINITIONS
#define NODEID                  16          //change this for each node
#define NETWORKID               100
#define GATEWAYID               20
#define FREQUENCY               RF69_433MHZ
#define ACK_TIME                500          // # of ms to wait for an ack
#define RETRY_NUMBER            2            // # of retries to attempt
#define SLEEP_PERIOD            SLEEP_4S
#define ONE_MIN_ARRAY_LEN       4
#define FIFTEEN_MIN_ARRAY_LEN   15
#define SLEEP_IN_SECONDS        60           //how long to sleep
#define POWER 4    //power digital output pin on digital 4
#define ANALOG_IN 0    //analog input pin on A0

DecaDuino DecaDuino(POWER,ANALOG_IN);

int sleepPeriod = 4;            //take number from SLEEP_PERIOD
int oneMinLen = ONE_MIN_ARRAY_LEN;
int fifteenMinLen = FIFTEEN_MIN_ARRAY_LEN;
int sleepTime = SLEEP_IN_SECONDS;


//variables for mcp342x
uint8_t address = 0x68;          //cannot change, set in hardware
MCP342x adc = MCP342x(address);


//var for moteino
byte sendSize = 0;
boolean requestACK = true;
byte errorCount = 0;
RFM69 radio;

//PAYLOAD STRUCTURE TO SEND DATA
typedef struct {
  byte              nodeId;
  long              sapFlow;
  byte              errorCount;
  int               decagon;
}
Payload;
Payload data;

//sapflow arrays
long oneMinVals[ONE_MIN_ARRAY_LEN];
long fifteenMinVals[FIFTEEN_MIN_ARRAY_LEN];
int  decaFifteenMinVals[FIFTEEN_MIN_ARRAY_LEN];
FastRunningMedian<long, FIFTEEN_MIN_ARRAY_LEN, 0> fifteenMinMedian;




//setup
void setup() {
  
  //Serial.begin(115200);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setHighPower(); //uncomment only for RFM69HW!
  data.nodeId = NODEID;
  Wire.begin();
  MCP342x::generalCallReset();
  delay(1); // MC342x needs 300us to settle, wait 1ms
  Wire.requestFrom(address, (uint8_t)1);
  if (!Wire.available()) {
    MCP342x::generalCallReset();
    delay(100);
    //Serial.print("No device found at address ");
    //Serial.println(address, HEX);
    
  }
  Blink(9,20);
}

//variables for loop:
long sapFlowValue, sapFlowMedian;
byte minuteCount;
int  decaValue, decaAverage;

//loop
void loop() {
  for (minuteCount = 0; minuteCount < fifteenMinLen; minuteCount++) {
    sapFlowValue = sapFlowRead();
    fifteenMinMedian.addValue(sapFlowValue);
    decaValue = DecaDuino.readADC();
    decaFifteenMinVals[minuteCount] = decaValue;
    sleepInSeconds(sleepTime);
  }
  sapFlowMedian = fifteenMinMedian.getMedian();
  decaAverage = arrayAverageInt(decaFifteenMinVals,fifteenMinLen);
  data.sapFlow = sapFlowMedian;
  data.errorCount = errorCount;
  data.decagon = decaAverage;
  errorCount = 0;
  for (byte i = 0; i < 5; i++) {
    if (radio.sendWithRetry(GATEWAYID, (const void*)(&data), sizeof(data), RETRY_NUMBER, ACK_TIME)) {
      //Blink(9, 5);      
      break;
    }
    else {
      sleepInSeconds(int(random(4,20)));
    }
  }
}


/*
FUNCTION sleepInSeconds
sleeps a set number of seconds at lower power consumption, must be at least four seconds
INPUT : integer of seconds
OUTPUT: none
*/

void sleepInSeconds(int seconds) {
  radio.sleep();
  int sleepIntervals = seconds / sleepPeriod;     //how many times are we going to sleep?
  for (byte i = 0; i < sleepIntervals; i++) {
    LowPower.powerDown(SLEEP_PERIOD, ADC_OFF, BOD_ON);
  }
}

/*
FUNCTION arrayAverage
averages the values of an array based on the number of elements in the array
INPUT : long array of values, int of number of elements of the array
OUTPUT: long average of array
*/
long arrayAverage(long * data, int count)
{
  int i;
  long total;
  long result;

  total = 0;
  for (i = 0; i < count; i++)
  {
    total = total + data[i];
  }
  result = total / count;
  return result;
}

/*
FUNCTION arrayAverage
averages the values of an array based on the number of elements in the array
INPUT : int array of values, int of number of elements of the array
OUTPUT: int average of array
*/
int arrayAverageInt(int * data, int count)
{
  int i;
  int total;
  int result;

  total = 0;
  for (i = 0; i < count; i++)
  {
    total = total + data[i];
  }
  result = total / count;
  return result;
}

/*
FUNCTION sapFlowRead
reads four values from the MCP3422 ADC and averages the results
INPUT : none
OUTPUT: unsigned int average of array
*/
long sapFlowRead(void) {
  long value;
  for (int i = 0; i < oneMinLen; i++) {
    MCP342x::Config status;
    uint8_t err = adc.convertAndRead(MCP342x::channel1, MCP342x::oneShot,
                                     MCP342x::resolution18, MCP342x::gain8,
                                     1000000, value, status);
    if (err) {
      MCP342x::generalCallReset();
      delay(1);
      errorCount++;
    }
    else {
      oneMinVals[i] = value;
      delay(10);
    }
  }
  long arrayAvg = arrayAverage(oneMinVals, oneMinLen);
  return arrayAvg;
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN, LOW);
}




