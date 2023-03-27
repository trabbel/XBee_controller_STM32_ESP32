#include <Arduino.h>
#include "utils.h"

int waitForByte(HardwareSerial &serial){
    int timeout = 0;
    while(!serial.available()>0){
        timeout++;
        if(timeout>1000){
            return 0;
        }
        delayMicroseconds(5);
    }
    return 1;
}