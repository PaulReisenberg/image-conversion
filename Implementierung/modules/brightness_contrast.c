#include "brightness_contrast.h"
#include <stdio.h>
#include <math.h>
#include "util.h"


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

int
brightness_contrast_V1(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                       float contrast, uint8_t *result) {

    // overflow check for width * height in size_t in main.c > readPPM()
    size_t wh = width * height;

    // normalise parameters
    // sum = 0 checked already in util.c > checkParams()
    float sum = a + b + c;
    a /= sum;
    b /= sum;
    c /= sum;

    float res;
    if (isnan(contrast)) {
        if (!brightness) {
            // Case 0: Grey Scale
            for (size_t i = 0; i < wh; i++) {
                res = (a * img[i * 3] + b * img[i * 3 + 1] + c * img[i * 3 + 2]);
                result[i] = (uint8_t) res;
            }
        } else {
            // Case 1: Grey Scale + Brightness
            for (size_t i = 0; i < wh; i++) {

                res = (a * img[i * 3] + b * img[i * 3 + 1] + c * img[i * 3 + 2]);
                res += brightness;
                if (res > 255) {
                    result[i] = 255;
                } else if (res < 0) {
                    result[i] = 0;
                } else {
                    result[i] = (uint8_t) res;
                }
            }
        }
    } else {
        double mean = 0.0;
        double var = 0.0;

        if (!brightness) {
            // Case 3: Grey Scale + Contrast
            for (size_t i = 0; i < wh; i++) {
                res = (a * img[i * 3] + b * img[i * 3 + 1] + c * img[i * 3 + 2]);
                result[i] = (uint8_t) res;
                mean += (uint8_t) res;
            }
        } else {
            // Case 2: Grey Scale + Brightness + Contrast
            for (size_t i = 0; i < wh; i++) {
                res = (a * img[i * 3] + b * img[i * 3 + 1] + c * img[i * 3 + 2]);
                res += brightness;
                if (res > 255) {
                    result[i] = 255;
                    mean += 255;
                } else if (res < 0) {
                    result[i] = 0;
                } else {
                    result[i] = (uint8_t) res;
                    mean += (uint8_t) res;
                }
            }
        }
        mean /= wh;

        // Calculate Variance
        for (size_t i = 0; i < wh; i++) {
            var += (result[i] - mean) * (result[i] - mean);
        }
        var /= wh;

        float kstd = 0.0;
        if (var != 0.0) {        // if var = 0 => kstd = 0 (Aufgabenstellung)
            kstd = contrast / sqrtHeron(var);                // k/std = k / sqrt(var)
            if (isnan(kstd) || isinf(kstd)) {
                fprintf(stderr, "computation for contrast failed\n");
                return 0;
            }
        }

        float summand = (1 - kstd) * mean;                // pre calculation of (1-kstd)*mean
        if (isnan(summand) || isinf(summand)) {
            fprintf(stderr, "computation for contrast failed\n");
            return 0;
        }

        // build lookup table for contrast adjustment
        uint8_t lookup[256];
        for (size_t i = 0; i < 256; i++) {
            res = kstd * i + summand;                    // new_val = kstd * pix + (1-kstd)*mean
            if (res > 255) {
                lookup[i] = 255;
            } else if (res < 0) {
                lookup[i] = 0;
            } else {
                lookup[i] = (uint8_t) res;
            }
        }
        //use lookup table to adjust contrast
        for (size_t i = 0; i < wh; i++) {
            result[i] = lookup[result[i]];
        }
    }
    return 1;
}


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

int
brightness_contrast_V2(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                       float contrast, uint8_t *result) {

    // overflow check for width * height in size_t in main.c > readPPM()
    size_t wh = width * height;

    float res;
    double mean = 0.0;
    double var = 0.0;

    // convert to grey scale
    for (size_t i = 0; i < wh; i++) {
        // overflow for 3*width*height checked in main.c > readPPM()
        res = (a * img[i * 3] + b * img[i * 3 + 1] + c * img[i * 3 + 2]) / (a + b + c);
        result[i] = (uint8_t) res;
    }

    //adjust brightness
    if (brightness) {
        for (size_t i = 0; i < wh; i++) {
            res = result[i] + brightness;
            if (res > 255) {
                result[i] = 255;
            } else if (res < 0) {
                result[i] = 0;
            } else {
                result[i] = (uint8_t) res;
            }
        }
    }

    if (!isnan(contrast)) {
        //calculate mean
        for (size_t i = 0; i < wh; i++) {
            mean += result[i];
        }
        mean /= wh;

        // calculate variance
        for (size_t i = 0; i < wh; i++) {
            var += (result[i] - mean) * (result[i] - mean);
        }
        var /= wh;

        float kstd = 0.0;
        if (var != 0.0) {        // if var = 0 => kstd = 0 (Aufgabenstellung)
            kstd = contrast / sqrtHeron(var);     // k/std = k / sqrt(var)
            if (isnan(kstd) || isinf(kstd)) {
                fprintf(stderr, "computation for contrast failed\n");
                return 0;
            }
        }

        float summand = (1 - kstd) * mean;                // pre calculation of (1-kstd)*mean
        if (isnan(summand) || isinf(summand)) {
            fprintf(stderr, "computation for contrast failed\n");
            return 0;
        }

        for (size_t i = 0; i < wh; i++) {
            res = kstd * result[i] + (1 - kstd) * mean;
            if (res > 255) {
                result[i] = 255;
            } else if (res < 0) {
                result[i] = 0;
            } else {
                result[i] = (uint8_t) res;
            }
        }
    }
    return 1;
}
