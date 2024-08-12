#include <stdint.h>
#include <stdio.h>

#ifndef TEAM120_BRIGHTNESS_CONTRAST_SSE_H
#define TEAM120_BRIGHTNESS_CONTRAST_SSE_H

/**
 * @brief Converts color coefficients to a scale with a maximum of 256.
 * 
 * @param a The coefficient for the red component.
 * @param b The coefficient for the green component.
 * @param c The coefficient for the blue component.
 * @param coeffs Pointer to an array where the scaled coefficients will be stored.
 * 
 * This function scales the given color coefficients so that their sum equals 256.
 * It is used to prepare the coefficients for SIMD operations that require integer
 * values. The function iteratively adjusts the coefficients to ensure they sum up
 * to the desired total while staying as close as possible to their original ratios.
 */

int brightness_contrast_V0(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness, float contrast, uint8_t *result);


#endif
