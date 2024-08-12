#ifndef TEAM120_UTIL_H
#define TEAM120_UTIL_H


/**
 * @brief Prints the help message for the program.
 */

void printHelp();


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

int checkParams(int V_option, int B_option,
                char *input_filename, char *output_filename,
                double a, double b, double c,
                int brightness, float contrast);


int parseAndStoreCoeffs(const char* str, float coeffs[3]);

int stringToLong(char* str, long* out);

int stringToInt(char* str, int* out);

float sqrtHeron(float n);

#endif