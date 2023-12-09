#include "SpotifyBase64.h"

/**
 * Base64 encoding and decoding of strings. Uses '-' for 62, '_' for 63, and no padding
 * 
 * This is a very stripped version of https://github.com/Densaugeo/base64_arduino.
 * All we need is the encoding functions and we don't want to expose too much.
 * We are using the URL versions of they're code, ultimately not using macros for that anymore.
 */

uint8_t SpotifyBase64::binaryToBase64(uint8_t v) {
    // Capital letters - 'A' is ascii 65 and base64 0
    if(v < 26) return v + 'A';
    
    // Lowercase letters - 'a' is ascii 97 and base64 26
    if(v < 52) return v + 71;
    
    // Digits - '0' is ascii 48 and base64 52
    if(v < 62) return v - 4;
    
    /* URL safe versions using '-' and '_' instead of '+', and '/' */
    
    // '-' is ascii 45 and base64 62
    if(v == 62) return '-';

    // '_' is ascii 95 and base64 62
    if(v == 63) return '_';
    
    return 64;
}

size_t SpotifyBase64::encode(const uint8_t input[], size_t inputLength, uint8_t output[]) {
    unsigned int full_sets = inputLength/3;
    
    // While there are still full sets of 24 bits...
    for(unsigned int i = 0; i < full_sets; ++i) {
        output[0] = binaryToBase64(                         input[0] >> 2);
        output[1] = binaryToBase64((input[0] & 0x03) << 4 | input[1] >> 4);
        output[2] = binaryToBase64((input[1] & 0x0F) << 2 | input[2] >> 6);
        output[3] = binaryToBase64( input[2] & 0x3F);
        
        input += 3;
        output += 4;
    }
    
    switch(inputLength % 3) {
    case 0:
        output[0] = '\0';
        break;
    case 1:
        output[0] = binaryToBase64(                         input[0] >> 2);
        output[1] = binaryToBase64((input[0] & 0x03) << 4);
        output[2] = '\0';

        /* We don't need the '=' at the end. */
        // output[2] = '=';
        // output[3] = '=';
        // output[4] = '\0';
        break;
    case 2:
        output[0] = binaryToBase64(                         input[0] >> 2);
        output[1] = binaryToBase64((input[0] & 0x03) << 4 | input[1] >> 4);
        output[2] = binaryToBase64((input[1] & 0x0F) << 2);
        output[3] = '\0';

        // output[3] = '=';
        // output[4] = '\0';
        break;
    }
  
  return Length(inputLength);
}