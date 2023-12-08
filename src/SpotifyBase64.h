/**
 * Base64 encoding and decoding of strings. Uses '+' for 62, '/' for 63, '=' for padding
 * 
 * This is a stripped version of https://github.com/Densaugeo/base64_arduino.
 * All we need is the encoding functions and we don't want to expose too much.
 * 
 */

#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED


/* encode_base64_length:
 *   Description:
 *     Calculates length of base64 string needed for a given number of binary bytes
 *   Parameters:
 *     input_length - Amount of binary data in bytes
 *   Returns:
 *     Number of base64 characters needed to encode input_length bytes of binary data
 */
static inline constexpr unsigned int SPOTIFY_ENCODE_BASE64_LENGTH(unsigned int input_length) {
  return (input_length + 2)/3*4;
}


#ifdef BASE64_SPOTIFY_ARDUINO_IMPLEMENTATION

unsigned char spotifyBinaryToBase64(unsigned char v) {
  // Capital letters - 'A' is ascii 65 and base64 0
  if(v < 26) return v + 'A';
  
  // Lowercase letters - 'a' is ascii 97 and base64 26
  if(v < 52) return v + 71;
  
  // Digits - '0' is ascii 48 and base64 52
  if(v < 62) return v - 4;
  
  #ifdef BASE64_SPOTIFY_ARDUINO_URL
  // '-' is ascii 45 and base64 62
  if(v == 62) return '-';
  #else
  // '+' is ascii 43 and base64 62
  if(v == 62) return '+';
  #endif

  #ifdef BASE64_SPOTIFY_ARDUINO_URL
  // '_' is ascii 95 and base64 62
  if(v == 63) return '_';
  #else
  // '/' is ascii 47 and base64 63
  if(v == 63) return '/';
  #endif
  
  return 64;
}

unsigned int spotifyEncodeBase64(const unsigned char input[], unsigned int input_length, unsigned char output[]) {
  unsigned int full_sets = input_length/3;
  
  // While there are still full sets of 24 bits...
  for(unsigned int i = 0; i < full_sets; ++i) {
    output[0] = spotifyBinaryToBase64(                         input[0] >> 2);
    output[1] = spotifyBinaryToBase64((input[0] & 0x03) << 4 | input[1] >> 4);
    output[2] = spotifyBinaryToBase64((input[1] & 0x0F) << 2 | input[2] >> 6);
    output[3] = spotifyBinaryToBase64( input[2] & 0x3F);
    
    input += 3;
    output += 4;
  }
  
  switch(input_length % 3) {
    case 0:
      output[0] = '\0';
      break;
    case 1:
      output[0] = spotifyBinaryToBase64(                         input[0] >> 2);
      output[1] = spotifyBinaryToBase64((input[0] & 0x03) << 4);
      output[2] = '\0';
      // output[2] = '=';
      // output[3] = '=';
      // output[4] = '\0';
      break;
    case 2:
      output[0] = spotifyBinaryToBase64(                         input[0] >> 2);
      output[1] = spotifyBinaryToBase64((input[0] & 0x03) << 4 | input[1] >> 4);
      output[2] = spotifyBinaryToBase64((input[1] & 0x0F) << 2);
      output[3] = '\0';
      // output[3] = '=';
      // output[4] = '\0';
      break;
  }
  
  return SPOTIFY_ENCODE_BASE64_LENGTH(input_length);
}
#endif // BASE64_SPOTIFY_ARDUINO_IMPLEMENTATION

#endif // ifndef
