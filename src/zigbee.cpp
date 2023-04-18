//#include "mbed.h"
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include "zigbee.h"
#include "utils.h"

#define START_DELIMITER 0x7E
#define ESCAPE_CHAR     0x7D

/**
 * Write a xBee API 2 frame in a buffer
 * 
 * This function assembles a 0x10 transmit request frame and writes it in a given buffer.
 * The frame can be send easily by writing the buffer to a serial port that is connected to 
 * a xBee device. 
 *
 * @param frame Pointer to char array, buffer where the frame is written
 * @param frame_id If frame id is 0x00, no acknowledgement is requested
 * @param addr16 16 bit local network address
 * @param addr64 64 bit MAC address
 * @param payload Pointer to the hex array to send as data
 * @param payloadSize Size of payload in bytes
 *
 * @returns Length of the constructed frame
 */
int writeFrame(char *frame, char frame_id, int addr16, uint64_t addr64, char *payload, int payloadSize){
    // Define a temporary array of sufficient size for everything that may be escaped
    char tempFrame[(16+payloadSize)];
    // Frame delimiter
    frame[0] = START_DELIMITER;
    // Payload length starting with frame type byte, excluding checksum and neccessary escape characters
    int frame_len = 0x0E + payloadSize;
    tempFrame[0] = (frame_len >> 8) & 0xFF;
    tempFrame[1] = frame_len & 0xFF;
    // API ID (TX request)
    tempFrame[2] = 0x10;
    // Frame ID
    tempFrame[3] = frame_id;//0x01;
    // 64-bit destination address
    for(int i = 0; i<8; i++){
        tempFrame[4+i] = (addr64 >> (7-i)*8) & 0xFF;
    }
    // 16-bit address
    tempFrame[12] = (addr16 >> 8) & 0xFF;
    tempFrame[13] = (addr16 >> 0) & 0xFF;
    // Broadcast radius
    tempFrame[14] = 0x00;
    // Options
    tempFrame[15] = 0x00;
    // Payload
    memcpy(&tempFrame[16], payload, payloadSize);
    // Checksum is calculated based on the original data (non-escaped payload),
    // starting with the frame type
    char checksum = 0x00;
    for(int i = 2; i< 16 + payloadSize; i++){
        checksum += tempFrame[i];
    }
    tempFrame[16+payloadSize] = 0xFF-checksum;
    // Escape the payload and count how many additional escape characters have to be transmitted
    int escapes = escapePayload(tempFrame, frame, payloadSize);

    /*for (int i = 0 ; i<18+payloadSize+escapes; i++){
            debug("%02x ", tempFrame[i]);
    }debug("\n");*/

    // The frame is now written in the buffer and ready to send
    return 18 + payloadSize + escapes;
}

/**
 * Escape control characters in tx frames
 *
 * Internal use only, is called by writeFrame() to escape characters on-the-fly
 * while writing them into the buffer. Everything after the start delimiter has to
 * be escaped, including the length bytes and checksum. Note that checksum and length
 * are calculated based on the original (unescaped) payload. To escape a character, 
 * 0x7D is written to the frame and then the original character XORed with 0x20.
 *
 * @param payload Pointer to char array that has to be escaped
 * @param tx_buf Pointer to buffer where to write the frame
 * @param payloadSize Size of payload in bytes
 *
 * @returns Number of added escape characters
 */
int escapePayload(char *payload, char *tx_buf, int payloadSize){
    // Counter for needed escape characters
    int escapes = 0;
    // Where to start writing the payload in the buffer
    int pos = 1;
    // Go through each payload byte
    for (int i = 0; i < 17 + payloadSize; i++){
        // Check if the current byte has to be escaped 
        // 0x7e         | frame delimiter, signifies a new frame
        // 0x7d         | escape character, signifies that the next character is escaped
        // 0x11, 0x13   | software control character
        if (payload[i] == 0x7E || payload[i] == 0x7D || payload[i] == 0x11 || payload[i] == 0x13){
            // If a byte has to be escaped, first write ox7d, followed by the byte XORed with 0x20
            tx_buf[pos++] = 0x7D;
            tx_buf[pos++] = payload[i] ^ 0x20;
            escapes++;
        }else{
            // Otherwise write the byte
            tx_buf[pos++] = payload[i];
        }
    }
    return escapes;
}

/**
 * Parse a XBee API 2 frame that is received over serial
 *
 * This method is called after the frame delimiter 0x7E is detected to parse the following frame.
 * It will write the payload in the given buffer and return a struct containing the number of
 * bytes in the payload as well as the frame type. In case of an error, like an unexpected frame
 * delimiter, no more serial data to read or a wrong checksum, this method returns {0x00, x}, 
 * which corresponds to frame tyoe 0x00 and the error code x. Frame type 0x00 does not exist in 
 * XBee API specifications and is thus used as an error identifier. The error codes correspdond to:
 *  0    | No more serial data to read
 * -1    | 0x7E detected
 * -2    | Wrong checksum
 *
 * @param frame Pointer to the rx buffer, where the payload of the message will be written
 * @param serial Serial RX port from where to read the data
 * @returns A parsedFrame struct, including the frame type and payload length
 */
parsedFrame readFrame(char *frame, HardwareSerial &serial){
    parsedFrame result = {0x00, 0};
    unsigned char lengthBytes[2];
    
    // Read the length bytes. Return an error if no data is available or
    // another frame delimiter is encountered
    for (int i = 0; i < 2; i++) {

        if(waitForByte(serial)==0){
            return result;
        }

        lengthBytes[i] = serial.read();

        if (lengthBytes[i] == START_DELIMITER) {
            result.length = -1;
            return result;
        }else if(lengthBytes[i] == ESCAPE_CHAR){
            // If an escape char is encountered, read the next byte instead and unescape it

            if(waitForByte(serial)==0){
                return result;
            }

            lengthBytes[i] = serial.read();
            lengthBytes[i] = lengthBytes[i] ^ 0x20;
        }
    }

    // The length defined by the length bytes is not equal to the actual amount
    // of bytes, escape characters are not counted
    int payloadSize = (lengthBytes[0] << 8) + lengthBytes[1];
    Serial.printf("S:%i\n", payloadSize);


    // Read the payload. Return an error if no data is available or
    // another frame delimiter is encountered
    for (int i = 0; i < payloadSize + 1; i++) {

        if(waitForByte(serial)==0){
            return result;
        }

        frame[i] = serial.read();
        if (frame[i] == START_DELIMITER) {
            result.length = -1;
            return result;
        }else if(frame[i] == ESCAPE_CHAR){
            // If an escape char is encountered, read the next byte instead and unescape it

            if(waitForByte(serial)==0){
                return result;
            }
            
            frame[i] = serial.read();
            frame[i] = frame[i] ^ 0x20;
        }
    }

    // Calculate the checksum, ignoring length bytes and escape characters
    char checksum = 0x00;
    for(int i = 0; i< payloadSize; i++){
        checksum += frame[i];
    }
    // Return an error if the checksum is not matching
    if (frame[payloadSize] != 0xFF - checksum){
        //debug("Checksum wrong! Should have been %02x, was %02x\n", frame[payloadSize], 0xFF -checksum);
        result.length = -2;
        return result;
    }

    result = {frame[0], payloadSize};
    return result;
}