#include <Arduino.h>
#include "utils.h"

/**
 * Wait for the next serial characters
 *
 * This method is called before serial.read if a serial character is expected.
 * It wait a certain time until serial is available and returns if it waits too 
 * long. 
 *
 * @param serial Serial RX port from where to read the data
 * @param timeoutCnt How many 5 microseconds delay to wait.
 *                   Default value is based on baud rate 115200
 * @returns 1 if serial is available, else 0
 */
int waitForByte(HardwareSerial &serial, int timeoutCnt /*= 1000*/){
    int timeout = 0;
    while(!serial.available()>0){
        timeout++;
        if(timeout>timeoutCnt){
            return 0;
        }
        delayMicroseconds(8);
    }
    return 1;
}