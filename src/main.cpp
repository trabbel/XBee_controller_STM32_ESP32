#include <Arduino.h>
#include <HardwareSerial.h>
//#include <SafeString.h>
#include <loopTimer.h>
#ifdef TARGET_STM_32
#include "stm32wbxx_hal.h"
#endif
#include "zigbee.h"

#define MAXIMUM_BUFFER_SIZE                                                 300
#define START_DELIMITER                                                     0x7E
#define RST_PIN                                                             PA0



char tx_buf[MAXIMUM_BUFFER_SIZE] = {0};
char rx_buf[MAXIMUM_BUFFER_SIZE] = {0};
int x = 0;


//millisDelay readDelay;
millisDelay sendDelay;



#ifdef TARGET_STM_32
HardwareSerial xbee(TX_PIN, RX_PIN);
#elif TARGET_ESP_32
HardwareSerial xbee(1);
#endif


static void rx_callback(char *buffer){

    int length = 0;
    char c;
    parsedFrame result;
    int timeoutCnt = 0;
    //while(1){
        // Wait until a frame delimiter arrives
        if(xbee.available()>0){
           // delay(1);
           // delay(1);
           // delay(1);
            c = 0;
            while(c != START_DELIMITER){
                c = xbee.read();
            }
            length = 0;
            // Parse the frame. In case of an error, a negative length
            // is returned
            //while(length <= 0){
                result = readFrame(buffer, xbee);
                length = result.length;
                Serial.printf("Payload Size: %i\n", length);
                /*timeoutCnt++;
                if(timeoutCnt > 3){
                    return;
                }
            }*/
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
    //}
    return;
}


int payloadSize;

void setup() {
    //char tx_buf[MAXIMUM_BUFFER_SIZE] = {0};
    //char rx_buf[MAXIMUM_BUFFER_SIZE] = {0};
    //char payload[] = "test~}{~test";
    char payload[] = "test~}{~testsuefiuwegifuwgefigweiurfz7489374t9394f983b49f8b5fg7893b64f934bz9823zbco8w8zeuboirzubwo84zbv97tz4bvuewzboruivb89346zbviuewzbort8ib34";

    Serial.begin(115200);
    //xbee.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    //xbee.begin(115200, SERIAL_8N1, 12, 13);
    #ifdef TARGET_STM_32
    xbee.begin(115200);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    #elif TARGET_ESP_32
    xbee.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    digitalWrite(15, LOW); 
    #endif
    // myled = 0;
    delay(100);
    #ifdef TARGET_STM_32
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    #elif TARGET_ESP_32
    digitalWrite(15, HIGH); 
    #endif
    delay(100);
    x = writeFrame(tx_buf, 0xFFFE, 0x0013a20041f217cc, payload, sizeof(payload)-1);
    // thread.start(callback(rx_callback, rx_buf));

    sendDelay.start(2000);
    Serial.printf("Setup finished\n");

}


void sendMessage() {
  if (sendDelay.justFinished()) {
    sendDelay.repeat(); // start delay again without drift
    Serial.printf("Send message\n");
    xbee.write(tx_buf, x);
  } // else nothing to do this call just return, quickly
}


/*int length = 0;
char c;
parsedFrame result;
int timeoutCnt;*/

void loop() {
    /*// myled = 1; 
    Serial.printf("Send message\n");
    xbee.write(tx_buf, x);
    if(xbee.available()>0){
        payloadSize = rx_callback(rx_buf);
    }
    // myled = 0; 
    //HAL_Delay(1000);
    delay(1000);*/
    sendMessage();
    rx_callback(rx_buf);


    /*
    if(xbee.available()>0){
        c = 0;
        while(c != START_DELIMITER){
            c = xbee.read();
        }
        length = 0;
        timeoutCnt = 0;
        // Parse the frame. In case of an error, a negative length
        // is returned
        while(length <= 0){
            result = readFrame(rx_buf, xbee);
            length = result.length;
            Serial.printf("Payload Size: %i\n", length);
            if(length == 0){
                timeoutCnt++;
            }
            if(timeoutCnt > 3){
                goto END;
            }
        }
        for (int i = 0 ; i<length; i++){
            Serial.printf("%02x", rx_buf[i]);
        }Serial.printf("\n");
        if(result.frameID == 0x90){
            Serial.printf("%.*s\n", length - 11, rx_buf + 11);
        }
    }
    END:
    ;*/
}
