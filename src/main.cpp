#include <Arduino.h>
#include <HardwareSerial.h>
#include <loopTimer.h>
#ifdef TARGET_STM_32
#include "stm32wbxx_hal.h"
#elif TARGET_ESP_32
#include "ESP32_LoRaWAN.h"
#endif
#include "zigbee.h"
#include "utils.h"
#define MAXIMUM_BUFFER_SIZE                                                 300
#define START_DELIMITER                                                     0x7E
#define RST_PIN                                                             PA0


#ifdef TARGET_STM_32
HardwareSerial xbee(TX_PIN, RX_PIN);
#elif TARGET_ESP_32
HardwareSerial xbee(1);


// ############# LoRaWAN if ESP32 ###########

/*license for Heltec ESP32 LoRaWan, quary your ChipID relevant license: http://resource.heltec.cn/search */
uint32_t  license[4] = {0x6CFFE668,0x0C49E20C,0x77B66ED9,0xC77E7FDA};

/* OTAA para*/
uint8_t DevEui[] = { 0x22, 0x32, 0x33, 0x00, 0x00, 0x88, 0x88, 0x02 };
uint8_t AppEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t AppKey[] = { 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x66, 0x01 };

/* ABP para*/
uint8_t NwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t AppSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t DevAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 8;

/*LoraWan debug level, select in arduino IDE tools.
* None : print basic info.
* Freq : print Tx and Rx freq, DR info.
* Freq && DIO : print Tx and Rx freq, DR, DIO0 interrupt and DIO1 interrupt info.
* Freq && DIO && PW: print Tx and Rx freq, DR, DIO0 interrupt, DIO1 interrupt and MCU deepsleep info.
*/
uint8_t debugLevel = 2;

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_EU868;

static void prepareTxFrame( uint8_t port )
{
    appDataSize = 4;//AppDataSize max value is 64 ( src/Commissioning.h -> 128 ) 
    appData[0] = 0x00;
    appData[1] = 0x01;
    appData[2] = 0x02;
    appData[3] = 0x03;
}





#endif







// Define RX and TX global buffer
char tx_buf[MAXIMUM_BUFFER_SIZE] = {0};
char rx_buf[MAXIMUM_BUFFER_SIZE] = {0};
int tx_length = 0;

// Delays for software-"multithreading"/scheduling
millisDelay sendDelay;






/*
############ TODO #############
Change void to int, return error codes
*/
static void rx_callback(char *buffer){

    int length = 0;
    char c;
    parsedFrame result;
    int timeoutCnt = 0;
    // Wait until a frame delimiter arrives
    if(xbee.available()>0){
        c = 0;
        while(c != START_DELIMITER){
            if(waitForByte(xbee)==0){
                return;
            }
            c = xbee.read();
        }
        length = 0;
        // Parse the frame. In case of an error, a negative length
        // is returned
        result = readFrame(buffer, xbee);
        length = result.length;
        Serial.printf("Payload Size: %i\n", length);

        if(length <= 0){
            return;
        }
        for (int i = 0 ; i<length; i++){
            Serial.printf("%02x", buffer[i]);
        }Serial.printf("\n");
        if(result.frameID == 0x90){
            Serial.printf("%.*s\n", length - 12, buffer + 12);
        }
    }
    return;
}


void setup() {
    char payload[] = "test~}{~testsuefiuwegifuwgefigweiurfz7489374t9394f983b49f8b5fg7893b64f934bz9823zbco8w8zeuboirzubwo84zbv97tz4bvuewzboruivb89346zbviuewzbort8ib34";
    //char payload[] = "test~}{~test";
    Serial.begin(115200);
    #ifdef TARGET_STM_32
    xbee.begin(115200);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    #elif TARGET_ESP_32
    xbee.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    digitalWrite(15, LOW); 


    // ################LoRaWAN#######################
    SPI.begin(SCK,MISO,MOSI,SS);
    Mcu.init(SS,RST_LoRa,DIO0,DIO1,license);
    deviceState = DEVICE_STATE_INIT;
    #endif


    // myled = 0;
    delay(100);
    #ifdef TARGET_STM_32
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    #elif TARGET_ESP_32
    digitalWrite(15, HIGH); 
    #endif
    delay(100);
    tx_length = writeFrame(tx_buf, 0xFFFE, 0x0013a20041f217cc, payload, sizeof(payload)-1);

    sendDelay.start(2000);
    Serial.printf("Setup finished\n");

}


void sendMessage() {
  if (sendDelay.justFinished()) {
    sendDelay.repeat();
    Serial.printf("Send message\n");
    xbee.write(tx_buf, tx_length);
  }
}


void loop() {
    sendMessage();
    rx_callback(rx_buf);

    #ifdef TARGET_ESP_32
    switch( deviceState ){
      case DEVICE_STATE_INIT:
      {
        #if(LORAWAN_DEVEUI_AUTO)
              LoRaWAN.generateDeveuiByChipID();
        #endif
        LoRaWAN.init(loraWanClass,loraWanRegion);
        break;
      }
      case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
      case DEVICE_STATE_SEND:
      {
        prepareTxFrame( appPort );
        LoRaWAN.send(loraWanClass);
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
      case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
      case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep(loraWanClass,debugLevel);
        break;
      }
      default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
  #endif
}