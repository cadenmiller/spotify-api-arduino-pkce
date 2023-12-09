#pragma once

#include <stddef.h>
#include <stdint.h>

class SpotifyBase64 {
public:

    /* encode_base64_length:
    *   Description:
    *     Calculates length of base64 string needed for a given number of binary bytes
    *   Parameters:
    *     input_length - Amount of binary data in bytes
    *   Returns:
    *     Number of base64 characters needed to encode input_length bytes of binary data
    */
    static inline constexpr size_t Length(size_t inputLength) {
        return (inputLength + 2)/3*4;
    }

    static size_t encode(const uint8_t input[], size_t input_length, uint8_t output[]);

private:
    static uint8_t binaryToBase64(uint8_t v);
};