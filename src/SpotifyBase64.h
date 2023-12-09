#pragma once

#include <stddef.h>
#include <stdint.h>

class SpotifyBase64 {
public:

    /** @brief Calculates length of base64 string needed for a given number of binary bytes
     *  @param length[in] Amount of bytes in your data.
     *  @return size_t Number of base64 characters needed to encode 'length' bytes of binary data
     */
    static inline constexpr size_t Length(size_t length) {
        return (length + 2)/3*4;
    }

    /** @brief URL safe base64 encoding for strings and data.
     *  @param input[in] Your input data.
     *  @param inputLength[in] Length in bytes of your input string to read.
     *  @param output[out] Writes to this buffer, must be at least @ref Length(inputLength).
     * 
     *  @warn This function will overflow your buffer if the output is not sized to Length(inputLength) or greater! 
     */
    static size_t encode(const uint8_t input[], size_t inputLength, uint8_t output[]);

private:
    static uint8_t binaryToBase64(uint8_t v);
};