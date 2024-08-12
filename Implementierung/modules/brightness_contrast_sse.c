#include <immintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "util.h"


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
 * to 256 while staying as close as possible to their original ratios.
 */

void convert_coeffs_to_max256(float a, float b, float c, uint16_t *coeffs) {

    float sum = a + b + c;
    // checked of sum == 0 in util.c > checkParams()
    coeffs[0] = 256 * (a / sum);
    coeffs[1] = 256 * (b / sum);
    coeffs[2] = 256 * (c / sum);

    while (1) {
        int total = coeffs[0] + coeffs[1] + coeffs[2];
        if (total == 256) {
            // return if sum of coefficients is 256
            return;
        } else {
            float a_diff = 256.0 * a / sum - coeffs[0];
            float b_diff = 256.0 * b / sum - coeffs[1];
            float c_diff = 256.0 * c / sum - coeffs[2];

            if (sum < 256) {
                // increase one coefficient by 1, if sum < 256
                if (a_diff > b_diff) {
                    if (a_diff > c_diff) {
                        coeffs[0]++;
                    } else {
                        coeffs[2]++;
                    }
                } else {
                    if (b_diff > c_diff) {
                        coeffs[1]++;
                    } else {
                        coeffs[2]++;
                    }
                }
            } else {
                // decrease one coefficient by 1, if sum > 256
                if (a_diff < b_diff) {
                    if (a_diff < c_diff) {
                        coeffs[0]--;
                    } else {
                        coeffs[2]--;
                    }
                } else {
                    if (b_diff < c_diff) {
                        coeffs[1]--;
                    } else {
                        coeffs[2]--;
                    }
                }
            }
        }
    }
}


/**
 * @brief Sums all elements of a 128-bit vector containing 16-bit integers.
 * 
 * @param vector The __m128i vector to be summed.
 * 
 * @return The sum of all eight 16-bit integers within the vector.
 */

int sum_mm_epi16(__m128i vector) {
    __m128i sum1 = _mm_add_epi16(vector, _mm_srli_si128(vector, 8));
    __m128i sum2 = _mm_add_epi16(sum1, _mm_srli_si128(sum1, 4));
    __m128i sum3 = _mm_add_epi16(sum2, _mm_srli_si128(sum2, 2));

    return _mm_extract_epi16(sum3, 0);
}


/**
 * @brief Loads image data and converts it to grayscale using SIMD operations.
 * 
 * @param img Pointer to the original image data.
 * @param i Index of the pixel to start at.
 * @param mask_red, mask_red2, mask_red3, mask_red4 Bitmasks used for extracting the red channel.
 * @param mask_green, mask_green2, mask_green3, mask_green4 Bitmasks used for extracting the green channel.
 * @param mask_blue, mask_blue2, mask_blue3, mask_blue4 Bitmasks used for extracting the blue channel.
 * @param a_coeff, b_coeff, c_coeff Coefficients for each color channel packed into __m128i vectors.
 * @param grey1, grey2 Pointers to __m128i vectors where the resulting grayscale values will be stored.
 *
 * This function loads three consecutive 16-byte segments from the image data,
 * corresponding to 16 pixels, and shuffles them to separate out the red, green,
 * and blue channels. Each channel is then multiplied by its respective coefficient
 * to convert the pixel values to grayscale. The results are stored in two __m128i
 * vectors representing the grayscale values of the 16 pixels.
 */

void load_and_convert_to_grey16(const uint8_t *img, size_t i,
                                __m128i mask_red, __m128i mask_red2, __m128i mask_red3, __m128i mask_red4,
                                __m128i mask_green, __m128i mask_green2, __m128i mask_green3, __m128i mask_green4,
                                __m128i mask_blue, __m128i mask_blue2, __m128i mask_blue3, __m128i mask_blue4,
                                __m128i a_coeff, __m128i b_coeff, __m128i c_coeff,
                                __m128i *grey1, __m128i *grey2) {

    // load pixels
    __m128i pixels1 = _mm_loadu_si128((__m128i *) (img + 3 * i));
    __m128i pixels2 = _mm_loadu_si128((__m128i *) (img + 3 * i + 16));
    __m128i pixels3 = _mm_loadu_si128((__m128i *) (img + 3 * i + 32));

    __m128i red1 = _mm_or_si128(_mm_shuffle_epi8(pixels1, mask_red), _mm_shuffle_epi8(pixels2, mask_red2));
    __m128i red2 = _mm_or_si128(_mm_shuffle_epi8(pixels2, mask_red3), _mm_shuffle_epi8(pixels3, mask_red4));

    __m128i green1 = _mm_or_si128(_mm_shuffle_epi8(pixels1, mask_green), _mm_shuffle_epi8(pixels2, mask_green2));
    __m128i green2 = _mm_or_si128(_mm_shuffle_epi8(pixels2, mask_green3), _mm_shuffle_epi8(pixels3, mask_green4));

    __m128i blue1 = _mm_or_si128(_mm_shuffle_epi8(pixels1, mask_blue), _mm_shuffle_epi8(pixels2, mask_blue2));
    __m128i blue2 = _mm_or_si128(_mm_shuffle_epi8(pixels2, mask_blue3), _mm_shuffle_epi8(pixels3, mask_blue4));

    // multiply color values by coefficients
    red1 = _mm_mullo_epi16(red1, a_coeff);
    green1 = _mm_mullo_epi16(green1, b_coeff);
    blue1 = _mm_mullo_epi16(blue1, c_coeff);

    red2 = _mm_mullo_epi16(red2, a_coeff);
    green2 = _mm_mullo_epi16(green2, b_coeff);
    blue2 = _mm_mullo_epi16(blue2, c_coeff);

    // sum color values to get grey scale and divide by 256
    *grey1 = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(red1, green1), blue1), 8);
    *grey2 = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(red2, green2), blue2), 8);
}


/**
 * @brief Performs brightness and contrast adjustment on an image using SIMD operations.
 * 
 * @param img Pointer to the original image data in uint8_t array.
 * @param width Width of the image.
 * @param height Height of the image.
 * @param a Coefficient for the red component in the grayscale conversion.
 * @param b Coefficient for the green component in the grayscale conversion.
 * @param c Coefficient for the blue component in the grayscale conversion.
 * @param brightness Brightness adjustment value.
 * @param contrast Contrast adjustment value.
 * @param result Pointer to the array where the adjusted image will be stored.
 * 
 * @return 1 if the operation was successful, 0 otherwise.
 *
 * The function first checks for overflow when calculating the size needed for the image.
 * Then it converts the provided coefficients to the [0,256] range and sets up the necessary
 * SIMD constants. Based on the presence of NaN in the contrast or zero in the brightness,
 * it handles four cases: grayscale conversion only, grayscale with brightness adjustment,
 * grayscale with contrast adjustment, and grayscale with both adjustments. The function
 * uses SIMD instructions to load and process blocks of pixels in parallel, applying the
 * grayscale conversion and brightness and contrast adjustments as needed. The contrast
 * adjustment uses a precomputed lookup table.
 */


int
brightness_contrast_V0(const uint8_t *img, size_t width, size_t height, float a, float b, float c, int16_t brightness,
                       float contrast, uint8_t *result) {

    //checked for overflow in util.c > checkParams()
    size_t wh = width * height;

    // convert parameters to range [0,256]
    uint16_t coeffs[3];
    convert_coeffs_to_max256(a, b, c, coeffs);

    // parameters stored as 16 bit integers
    __m128i a_coeff = _mm_set1_epi16(coeffs[0]);
    __m128i b_coeff = _mm_set1_epi16(coeffs[1]);
    __m128i c_coeff = _mm_set1_epi16(coeffs[2]);

    // bit masks for shuffling rgb values 
    __m128i mask_red1 = _mm_setr_epi8(0, -1, 3, -1, 6, -1, 9, -1, 12, -1, 15, -1, -1, -1, -1, -1);
    __m128i mask_green1 = _mm_setr_epi8(1, -1, 4, -1, 7, -1, 10, -1, 13, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_blue1 = _mm_setr_epi8(2, -1, 5, -1, 8, -1, 11, -1, 14, -1, -1, -1, -1, -1, -1, -1);

    __m128i mask_red2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, 5, -1);
    __m128i mask_green2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 3, -1, 6, -1);
    __m128i mask_blue2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, 4, -1, 7, -1);

    __m128i mask_red3 = _mm_setr_epi8(8, -1, 11, -1, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_green3 = _mm_setr_epi8(9, -1, 12, -1, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_blue3 = _mm_setr_epi8(10, -1, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    __m128i mask_red4 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, 1, -1, 4, -1, 7, -1, 10, -1, 13, -1);
    __m128i mask_green4 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, 2, -1, 5, -1, 8, -1, 11, -1, 14, -1);
    __m128i mask_blue4 = _mm_setr_epi8(-1, -1, -1, -1, 0, -1, 3, -1, 6, -1, 9, -1, 12, -1, 15, -1);

    // bit masks for combining two sets of grey values 
    __m128i mask_grey = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1);
    __m128i mask_grey2 = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14);

    // additional __m128i variables for pixel conversion
    __m128i zero = _mm_setzero_si128();
    __m128i max = _mm_set1_epi16(255);
    __m128i brightness_vector = _mm_set1_epi16((short) brightness);
    __m128i mean_vector = _mm_setzero_si128();

    __m128i grey1;
    __m128i grey2;
    int16_t res;

    if (isnan(contrast)) {
        if (!brightness) {
            // Case 0: Grey Scale
            for (size_t i = 0; i < wh - (wh % 16); i += 16) {
                // convert next 16 pixels to grey -> grey1, grey2
                load_and_convert_to_grey16(img, i,
                                           mask_red1, mask_red2, mask_red3, mask_red4,
                                           mask_green1, mask_green2, mask_green3, mask_green4,
                                           mask_blue1, mask_blue2, mask_blue3, mask_blue4,
                                           a_coeff, b_coeff, c_coeff,
                                           &grey1, &grey2);

                // combine first 8 and last 8 grey values and store result
                grey1 = _mm_shuffle_epi8(grey1, mask_grey);
                grey2 = _mm_shuffle_epi8(grey2, mask_grey2);
                _mm_storeu_si128((__m128i *) (result + i), _mm_or_si128(grey1, grey2));
            }
            // process remaining pixels after SIMD
            for (size_t i = wh - (wh % 16); i < wh; i++) {
                res = (coeffs[0] * img[i * 3] + coeffs[1] * img[i * 3 + 1] + coeffs[2] * img[i * 3 + 2]) / 256;
                result[i] = (uint8_t) res;
            }
        } else {
            for (size_t i = 0; i < wh - (wh % 16); i += 16) {
                // convert next 16 pixels to grey -> grey1, grey2
                load_and_convert_to_grey16(img, i,
                                           mask_red1, mask_red2, mask_red3, mask_red4,
                                           mask_green1, mask_green2, mask_green3, mask_green4,
                                           mask_blue1, mask_blue2, mask_blue3, mask_blue4,
                                           a_coeff, b_coeff, c_coeff,
                                           &grey1, &grey2);

                // add brightness to grey values
                grey1 = _mm_add_epi16(grey1, brightness_vector);
                grey2 = _mm_add_epi16(grey2, brightness_vector);

                // clamp values in [0,255]
                grey1 = _mm_min_epi16(max, grey1);
                grey1 = _mm_max_epi16(zero, grey1);
                grey2 = _mm_min_epi16(max, grey2);
                grey2 = _mm_max_epi16(zero, grey2);

                // combine first 8 and last 8 grey values and store result
                grey1 = _mm_shuffle_epi8(grey1, mask_grey);
                grey2 = _mm_shuffle_epi8(grey2, mask_grey2);
                _mm_storeu_si128((__m128i *) (result + i), _mm_or_si128(grey1, grey2));
            }
            // process remaining pixels after SIMD
            for (size_t i = wh - (wh % 16); i < wh; i++) {
                res = (coeffs[0] * img[i * 3] + coeffs[1] * img[i * 3 + 1] + coeffs[2] * img[i * 3 + 2]) / 256;
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
        double mean = 0;
        double var = 0;

        if (!brightness) {
            // Case 2: Grey Scale + Contrast
            for (size_t i = 0; i < wh - (wh % 16); i += 16) {
                // convert next 16 pixels to grey -> grey1, grey2
                load_and_convert_to_grey16(img, i,
                                           mask_red1, mask_red2, mask_red3, mask_red4,
                                           mask_green1, mask_green2, mask_green3, mask_green4,
                                           mask_blue1, mask_blue2, mask_blue3, mask_blue4,
                                           a_coeff, b_coeff, c_coeff,
                                           &grey1, &grey2);

                // calculate mean by summing grey values
                mean_vector = _mm_add_epi16(mean_vector, grey1);
                mean_vector = _mm_add_epi16(mean_vector, grey2);

                // store sum in mean and reset mean_vector to avoid overflow
                if (i % 128 == 0) {
                    mean += sum_mm_epi16(mean_vector);
                    mean_vector = _mm_setzero_si128();
                }
                grey1 = _mm_shuffle_epi8(grey1, mask_grey);
                grey2 = _mm_shuffle_epi8(grey2, mask_grey2);
                _mm_storeu_si128((__m128i *) (result + i), _mm_or_si128(grey1, grey2));
            }
            mean += sum_mm_epi16(mean_vector);

            // process remaining pixels after SIMD
            for (size_t i = wh - (wh % 16); i < wh; i++) {
                res = (coeffs[0] * img[i * 3] + coeffs[1] * img[i * 3 + 1] + coeffs[2] * img[i * 3 + 2]) / 256;
                result[i] = (uint8_t) res;
                mean += res;
            }
        } else {
            // Case 3: Grey Scale + Brightness + Contrast
            for (size_t i = 0; i < wh - (wh % 16); i += 16) {
                // convert next 16 pixels to grey -> grey1, grey2
                load_and_convert_to_grey16(img, i,
                                           mask_red1, mask_red2, mask_red3, mask_red4,
                                           mask_green1, mask_green2, mask_green3, mask_green4,
                                           mask_blue1, mask_blue2, mask_blue3, mask_blue4,
                                           a_coeff, b_coeff, c_coeff,
                                           &grey1, &grey2);

                // add brightness
                grey1 = _mm_add_epi16(grey1, brightness_vector);
                grey2 = _mm_add_epi16(grey2, brightness_vector);

                // clamp values in [0,255]
                grey1 = _mm_min_epi16(max, grey1);
                grey1 = _mm_max_epi16(zero, grey1);
                grey2 = _mm_min_epi16(max, grey2);
                grey2 = _mm_max_epi16(zero, grey2);

                // calculate mean by summing grey values
                mean_vector = _mm_add_epi16(mean_vector, grey1);
                mean_vector = _mm_add_epi16(mean_vector, grey2);

                // store sum in mean and reset mean_vector to avoid overflow
                if (i % 128 == 0) {
                    mean += sum_mm_epi16(mean_vector);
                    mean_vector = _mm_setzero_si128();
                }

                // combine first 8 and last 8 grey values and store result 
                grey1 = _mm_shuffle_epi8(grey1, mask_grey);
                grey2 = _mm_shuffle_epi8(grey2, mask_grey2);
                _mm_storeu_si128((__m128i *) (result + i), _mm_or_si128(grey1, grey2));
            }
            mean += sum_mm_epi16(mean_vector);

            // convert remaining pixels 
            for (size_t i = wh - (wh % 16); i < wh; i++) {
                res = (coeffs[0] * img[i * 3] + coeffs[1] * img[i * 3 + 1] + coeffs[2] * img[i * 3 + 2]) / 256;
                res += brightness;
                if (res > 255) {
                    result[i] = 255;
                    mean += 255;
                } else if (res < 0) {
                    result[i] = 0;
                } else {
                    result[i] = (uint8_t) res;
                    mean += res;
                }
            }
        }
        mean /= wh;

        // calcualte variance		
        __m128 neg_mean_vec = _mm_set1_ps((float) (-1.0 * mean));
        __m128 var_vec = _mm_set1_ps(0.0f);
        __m128 acc;
        __m128 pix_flt;

        for (size_t i = 0; i < wh - (wh % 16); i += 16) {
            // load next 16 grey values
            __m128i pixels = _mm_loadu_si128((__m128i *) (result + i));
            for (size_t j = 0; j < 4; j++) {

                // load 4 grey values as float into pix_flt -> 4 iterations
                pix_flt = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(pixels));

                // acc = grey_val - mean
                acc = _mm_add_ps(pix_flt, neg_mean_vec);

                // acc = (grey_val - mean)^2
                acc = _mm_mul_ps(acc, acc);
                var_vec = _mm_add_ps(var_vec, acc);

                // shift left to process next 4 bytes in next round
                pixels = _mm_srli_si128(pixels, 4);
            }
        }
        var_vec = _mm_hadd_ps(var_vec, var_vec);
        var_vec = _mm_hadd_ps(var_vec, var_vec);
        var = _mm_cvtss_f32(var_vec);

        // process remaining grey values for the variance after SIMD
        for (size_t i = wh - (wh % 16); i < wh; i++) {
            var += (result[i] - mean) * (result[i] - mean);
        }
        var /= wh;

        float kstd = 0.0;
        if (var != 0.0) {        // if var = 0 => kstd = 0 (Aufgabenstellung)
            kstd = contrast / sqrtHeron(var);       // k/std = k / sqrt(var)
            if (isnan(kstd) || isinf(kstd)) {
                fprintf(stderr, "computation for contrast failed\n");
                return 0;
            }
        }

        float summand = (1 - kstd) * mean;         // pre calculation of (1-kstd)*mean
        if (isnan(summand) || isinf(summand)) {
            fprintf(stderr, "computation for contrast failed\n");
            return 0;
        }

        // Adjust contrast with lookup table
        uint8_t lookup[256];
        for (size_t i = 0; i < 256; i++) {
            res = kstd * i + summand;   // new_val = kstd * pix + (1-kstd)*mean
            if (res > 255) {
                lookup[i] = 255;
            } else if (res < 0) {
                lookup[i] = 0;
            } else {
                lookup[i] = (uint8_t) res;
            }
        }
        for (size_t i = 0; i < wh; i++) {
            result[i] = lookup[result[i]];
        }
    }
    return 1;
}