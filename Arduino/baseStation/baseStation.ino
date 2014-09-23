#include <RFM69.h>
#include <SPI.h>
#include <Wire.h>
#include <MCP342x.h>

#define NODEID         20
#define NETWORKID      100
#define FREQUENCY      RF69_433MHZ 
#define SERIAL_BAUD    115200
#define SAP_ARRAY_LEN  3                   //how many sap flow values to read each time data is received
#define LED            9

//moteino variables
int arrayLen = SAP_ARRAY_LEN;
RFM69 radio;
long sapVals[SAP_ARRAY_LEN];

//variables for mcp342x
uint8_t address = 0x68;          //cannot change, set in hardware
MCP342x adc = MCP342x(address);

//payload to receive data
typedef struct {
  byte   nodeId;        //sender ID
  long   sapFlow;       //sapFlow value
  byte   errorCount;    //ADC error count
  int    decagon;      
} Payload;
Payload data;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
  radio.setHighPower();
  Wire.begin();
  MCP342x::generalCallReset();
  delay(1); // MC342x needs 300us to settle, wait 1ms
  Wire.requestFrom(address, (uint8_t)1);
  data.decagon = 0;
}

int  rssi;
long sapFlowValue;

void loop() {
  if (radio.receiveDone())
  {
    
    rssi = radio.RSSI;
    data = *(Payload*)radio.DATA; 
    if (radio.ACK_REQUESTED)
    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();
    }
    Serial.println(data.nodeId);
    Serial.println(data.sapFlow);
    Serial.println(data.errorCount);
    Serial.println(rssi);
    sapFlowValue = sapFlowRead();
    Serial.println(sapFlowValue);
    Serial.println(data.decagon);
    Blink(LED, 3);
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN, LOW);
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
FUNCTION sapFlowRead
reads four values from the MCP3422 ADC and averages the results
INPUT : none
OUTPUT: unsigned int average of array
*/
long sapFlowRead(void) {
  long value;
  for (int i = 0; i < arrayLen; i++) {
    MCP342x::Config status;
    uint8_t err = adc.convertAndRead(MCP342x::channel1, MCP342x::oneShot,
                                     MCP342x::resolution18, MCP342x::gain8,
                                     1000000, value, status);
    if (err) {
      MCP342x::generalCallReset();
      delay(1);
      
    }
    else {
      sapVals[i] = value;
      delay(10);
    }
  }
  long arrayAvg = arrayAverage(sapVals, arrayLen);
  return arrayAvg;
}
