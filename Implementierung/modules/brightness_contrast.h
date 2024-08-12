#include <stdint.h>
#include <stdio.h>

#ifndef TEAM120_BRIGHTNESS_CONTRAST_H
#define TEAM120_BRIGHTNESS_CONTRAST_H

/**
 * @brief Adjusts the brightness and contrast of an image using Version 1 implementation.
 * 
 * @param img Pointer to the original image data in uint8_t array.
 * @param width The width of the image.
 * @param height The height of the image.
 * @param a The coefficient for the red component in the grayscale conversion.
 * @param b The coefficient for the green component in the grayscale conversion.
 * @param c The coefficient for the blue component in the grayscale conversion.
 * @param brightness The brightness adjustment value.
 * @param contrast The contrast adjustment value.
 * @param result Pointer to the array where the adjusted image will be stored.
 * 
 * @return Returns 1 on successful completion, 0 on failure.
 *
 * This function processes an image to adjust its brightness and/or contrast based on provided parameters.
 * It first ensures that there are no overflows in the calculations of image sizes.
 * The coefficients for grayscale conversion are normalized so their sum equals 1.
 * Depending on whether the contrast is set to NaN and brightness to 0, the function
 * handles four different cases: grayscale conversion only, grayscale with brightness,
 * grayscale with contrast, and grayscale with both brightness and contrast.
 * If the computation for contrast fails, it will output an error message and return 0.
 * The function uses a lookup table for efficient contrast adjustment.
 */

int brightness_contrast_V1(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness, float contrast, uint8_t *result);


/**
 * @brief Naive implementation for adjusting the brightness and contrast of an image.
 * 
 * @param img Pointer to the original image data in uint8_t array.
 * @param width The width of the image.
 * @param height The height of the image.
 * @param a The coefficient for the red component in the grayscale conversion.
 * @param b The coefficient for the green component in the grayscale conversion.
 * @param c The coefficient for the blue component in the grayscale conversion.
 * @param brightness The brightness adjustment value.
 * @param contrast The contrast adjustment value.
 * @param result Pointer to the array where the adjusted image will be stored.
 * 
 * @return Returns 1 on successful completion, 0 on failure.
 */

int brightness_contrast_V2(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness, float contrast, uint8_t *result);

#endif
