#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

/**
 * @brief Prints the help message for the program.
 */

void printHelp() {
    printf("Usage:\n"
           "  program_name input_file [options] \n\n"
           "Options:\n"
           "  -o <file>\t\t Specifies output file path (default: output.pgm)\n"
           "  -V <val>\t\t Use variant <val> (integer) of the algorithm (default: 0).\n"
           "  -B <val>\t\t Measures the runtime of the specified implementation. The optional argument <val> (integer) specifies the number of repetitions of the function call.\n"
           "  --coeffs <a,b,c>\t Specify coefficients for grayscale conversion (default: 0.21,0.72,0.07).\n"
           "  --brightness <val>\t Adjust brightness by <val> (integer).\n"
           "  --contrast <val>\t Adjust contrast by <val> (integer).\n"
           "  -h, --help\t\t Display this help and exit.\n\n"
           "Description:\n"
           "This program converts PPM (P6 format) images to grayscale PGM images. It allows adjustment of brightness and contrast.\n"
           "The grayscale conversion uses the specified coefficients for the red, green, and blue channels.\n"
           "Brightness and contrast adjustments are optional.\n"
           "The program supports three variants of the algorithm: V0, V1, and V2.\n\n"
           "Examples:\n"
           "  program_name input.ppm -o output.pgm\n"
           "  program_name input.ppm -o output.pgm --coeffs 0.3,0.59,0.11 --brightness 20 --contrast 10\n"
           "  program_name input.ppm -o output.pgm -V 1 -B 2\n\n");
}


/**
 * @brief Validates the input parameters provided to the program.
 * 
 * @param V_option The version of the algorithm to use.
 * @param B_option The number of repetitions for the function call.
 * @param input_filename The name of the input file.
 * @param output_filename The name of the output file.
 * @param a The coefficient for the red component.
 * @param b The coefficient for the green component.
 * @param c The coefficient for the blue component.
 * @param brightness The brightness adjustment value.
 * @param contrast The contrast adjustment value.
 * 
 * @return 1 if all parameters are valid, 0 otherwise.
 */

int MAX_VERSION = 2;

int checkParams(int V_option, int B_option, char *input_filename, char *output_filename, double a, double b, double c,
                int brightness, float contrast) {

    if (brightness < -255 || brightness > 255) {
        fprintf(stderr, "The brightness must be in [-255, 255].\n");
        return 0;
    }

    if (!isnan(contrast)) {
        if (contrast < -255 || contrast > 255) {
            fprintf(stderr, "The contrast must be in [-255, 255].\n");
            return 0;
        }
    }

    if (a < 0 || b < 0 || c < 0) {
        fprintf(stderr, "The coefficients must greater or equal to zero.\n");
        return 0;
    }

    if (V_option < 0 || V_option > MAX_VERSION) {
        fprintf(stderr, "Version %d does not exist. Choose a version from [0, %d].\n", V_option, MAX_VERSION);
        return 0;
    }

    if (B_option < 0) {
        fprintf(stderr, "Option -B can not be negative.\n");
        return 0;
    }

    if (!output_filename) {
        fprintf(stderr, "Output filename has to be set.\n");
        return 0;
    }

    if (!input_filename) {
        fprintf(stderr, "Input filename has to be set.\n");
        return 0;
    }

    if (a + b + c == 0.0) {
        fprintf(stderr, "Sum of coefficients can not be 0.\n");
        return 0;
    }

    return 1;
}


/**
 * @brief Parses a string of grayscale conversion coefficients and stores them.
 * 
 * @param str The string containing the coefficients, separated by commas.
 * @param coeffs An array to store the parsed coefficients.
 * 
 * @return 1 if the parsing is successful, 0 otherwise.
 */

int parseAndStoreCoeffs(const char *str, float coeffs[3]) {
    
    if (coeffs == NULL || str == NULL) {
        return 0;
    }
 
    char extraChar;
    int numParsed = sscanf(str, "%f,%f,%f%c", &coeffs[0], &coeffs[1], &coeffs[2], &extraChar);

    // Check if exactly three coefficients were  parsed
    if (numParsed != 3) {
        fprintf(stderr, "Error: Invalid format of coefficients. Expected format: '1.0,2.0,3.0'\n");
        return 0;
    }
    return 1;
}

/**
 * @brief Converts a string to a long integer.
 * 
 * @param str The string to convert.
 * @param out Pointer to the long integer to store the result.
 * 
 * @return 1 if the conversion is successful, 0 otherwise.
 */
int stringToLong(char *str, long *out) {

    if (str == NULL || out == NULL) {
        return 0;
    }

    char *endptr;
    errno = 0;

    long result = strtol(str, &endptr, 10);

    if (endptr == str || *endptr) {
        return 0;
    }

    if (errno == ERANGE) {
        return 0;
    }

    *out = result;
    return 1;
}

/**
 * @brief Converts a string to an integer.
 * 
 * @param str The string to convert.
 * @param out Pointer to the integer to store the result.
 * 
 * @return 1 if the conversion is successful, 0 otherwise.
 */

int stringToInt(char *str, int *out) {

    if(str == NULL || out == NULL) {
        return 0;
    }

    long outLong;
    if (stringToLong(str, &outLong)) {
        if (outLong < INT_MIN || outLong > INT_MAX) {
            return 0;
        }

        *out = (int) outLong;
        return 1;
    }
    return 0;
}

/**
 * @brief Calculates the square root of a number using Heron's method.
 * 
 * @param n The number to calculate the square root of.
 * 
 * @return The square root of n.
 */

float sqrtHeron(float n) {
 
    if (n < 0 || isnan(n) || isinf(n))
        return NAN;

    if (n == 0.0)
        return 0;

    float x = (n + 1) / 2;
    float x0 = 0;
    while (x != x0) {
        x0 = x;
        x = (x + (n / x)) / 2;
    }
    return x;
}