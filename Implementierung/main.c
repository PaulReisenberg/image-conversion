#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "modules/brightness_contrast.h"
#include "modules/brightness_contrast_sse.h"
#include "modules/util.h"


typedef struct {
    size_t width, height;
    uint8_t *data;
} PPMImage;

/**
 * @brief Reads a PPM image from a file.
 *
 * @param filename The path to the PPM file to be read.
 *
 * @return A pointer to a dynamically allocated PPMImage structure containing
 *         the image data. If the file cannot be opened, is not in the correct
 *         format, or if there is a memory allocation failure, the program
 *         will terminate with an error message.
 *
 * This function opens a PPM file and reads its contents into a PPMImage structure.
 * The function expects the PPM file to be in the P6 format.
 * It performs several checks to ensure the format is correct, including checking
 * the file header (must be 'P6'), the image dimensions, and the maximum color value.
 * The function handles whitespace and comments in the PPM file format.
 */

PPMImage *readPPM(const char *filename) {

    int buflen = 64;
    char buffer[buflen];
    PPMImage *img;
    FILE *fp;
    long maxval = -1;


    // open file
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    img = (PPMImage *) malloc(sizeof(PPMImage));
    if (!img) {
        fprintf(stderr, "Failed to allocate memeory for image\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    // load file format into buffer and ignore comments
    char curr = 'x';            // Current character
    int numWP = 0;                // Number of whitespaces read. Concurrent whitespaces are counted as 1
    int token_switch = 0;        // 0: currently reading token 1: between tokens

    for (int i = 0; i < buflen - 1 || numWP >= 3; ++i) {
        if (!fread(&curr, 1, 1, fp)) {
            fprintf(stderr, "Error reading file.\n");
            free(img);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        if (token_switch) {
            if (curr == ' ' || curr == '\t' || curr == '\r' || curr == '\n') {
                // Whitespace character between tokens are ignored, writing index has to be 0 in next round
                i = -1;
            } else if (curr == '#') {
                // Ignore comment and decrement writing index
                while (curr != '\r' && curr != '\n') {
                    if (!fread(&curr, 1, 1, fp)) {
                        fprintf(stderr, "Error reading file.\n");
                        free(img);
                        fclose(fp);
                        exit(EXIT_FAILURE);
                    }
                }
                i = -1;
            } else {
                // Whitespace between tokens has ended. Start reading token
                token_switch = 0;
            }
        }
        // continue to next iteration if still between tokens
        if (token_switch) {
            continue;
        }

        if (curr == '#') {
            // Ignore everything in the comment

            while (curr != '\r' && curr != '\n') {
                if (!fread(&curr, 1, 1, fp)) {
                    fprintf(stderr, "Error reading file.\n");
                    free(img);
                    fclose(fp);
                    exit(EXIT_FAILURE);
                }
            }
            i--;
        } else {

            if (curr == ' ' || curr == '\t' || curr == '\r' || curr == '\n') {    //token has ended

                buffer[i] = '\0';
                if (numWP == 0) {    //current token: image format

                    if (buffer[0] != 'P' || buffer[1] != '6') {
                        fprintf(stderr, "Format of ppm must be P6\n");
                        free(img);
                        fclose(fp);
                        exit(EXIT_FAILURE);
                    }
                } else if (numWP == 1) {    //current token: width

                    long width;
                    int check = stringToLong(buffer, &width);
                    if (!check || width <= 0) {
                        fprintf(stderr, "Invalid image width\n");
                        fclose(fp);
                        free(img);
                        exit(EXIT_FAILURE);
                    }
                    img->width = (size_t) width;
                } else if (numWP == 2) {    //current token: height
                    long height;
                    int check = stringToLong(buffer, &height);
                    if (!check || height <= 0) {
                        fprintf(stderr, "Invalid image height\n");
                        fclose(fp);
                        free(img);
                        exit(EXIT_FAILURE);
                    }
                    img->height = (size_t) height;
                } else if (numWP == 3) {    // curent token: maximum_value

                    if (stringToLong(buffer, &maxval) != 1) {
                        fprintf(stderr, "Invalid max color value (error loading '%s')\n", filename);
                        fclose(fp);
                        free(img);
                        exit(EXIT_FAILURE);
                    }
                    if (maxval >= 256 || maxval <= 0) {
                        fprintf(stderr, "Invalid maximum value\n");
                        fclose(fp);
                        free(img);
                        exit(EXIT_FAILURE);
                    }
                    break;

                } else {
                    fprintf(stderr, "Error parsing image\n");
                    fclose(fp);
                    free(img);
                    exit(EXIT_FAILURE);
                }

                token_switch = 1;        // token has ended
                i = -1;
                numWP++;
            } else {
                buffer[i] = curr;
            }
        }
    }
    //check if 4 tokens have been parsed (P6, width, height, maxval)
    if (numWP != 3) {
        fprintf(stderr, "Image format corrupted\n");
        fclose(fp);
        free(img);
        exit(EXIT_FAILURE);
    }

    // check for overflow: 3*width*height
    size_t three_height;
    size_t pix_mem_size;
    if (__builtin_umull_overflow(3, img->height, &three_height)) {
        fprintf(stderr, "Image too big\n");
        fclose(fp);
        free(img);
        exit(EXIT_FAILURE);
    }
    if (__builtin_umull_overflow(three_height, img->width, &pix_mem_size)) {
        fprintf(stderr, "image too big\n");
        fclose(fp);
        free(img);
        exit(EXIT_FAILURE);
    }

    // allocate memory for rgb values
    img->data = (uint8_t *) malloc(pix_mem_size);

    if (!img->data) {
        fprintf(stderr, "Unable to allocate memory for image\n");
        fclose(fp);
        free(img);
        exit(EXIT_FAILURE);
    }

    // load rgb values into img->data
    if (fread(img->data, three_height, img->width, fp) != img->width) {
        fprintf(stderr, "Error loading image data from '%s'\n", filename);
        fclose(fp);
        free(img->data);
        free(img);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    return img;
}

/**
 * @brief Writes a PGM image to a file.
 *
 * @param output_filename The path where the PGM file will be written.
 * @param pixels Pointer to the array of pixels that constitute the image.
 * @param width The width of the image.
 * @param height The height of the image.
 *
 * This function creates a PGM file and writes the image data into it.
 * The image is written in P5 format. In case of failure to open the file for writing,
 * the program terminates with an error message.
 */
int writePGM(const char *output_filename, const uint8_t *pixels, size_t width, size_t height) {
    FILE *fp;

    fp = fopen(output_filename, "wb");

    if (fp == NULL) {
        fprintf(stderr, "Unable to open file '%s' for writing\n", output_filename);
        return 0;
    }

    fprintf(fp, "P5\n%zu %zu\n255\n", width, height);

    // overflow checked already
    fwrite(pixels, sizeof(uint8_t), width * height, fp);
    fclose(fp);
    return 1;
}

/**
 * @brief Frees the memory allocated for a PPMImage structure.
 *
 * @param img Pointer to the PPMImage structure to be freed.
 */

void freePPM(PPMImage *img) {
    free(img->data);
    free(img);
}


/**
 * @brief Main function of the program.
 */

int main(int argc, char **argv) {

    int opt;
    int V_option = 0;
    int B_option = 0;
    int brightness = 0;
    int tmp_contrast;
    float contrast = NAN;                       // nan if user does not what to adjust the contrast
    char *input_filename;
    char *output_filename = "output.pgm";       // standard output file

    float coeffs[3];
    coeffs[0] = 0.21484375;                     // standard coeff a
    coeffs[1] = 0.7109375;                      // standard coeff b
    coeffs[2] = 0.07421875;                     // standard coeff c

    struct option long_options[] = {
            {"coeffs",     required_argument, 0, 'c'},
            {"brightness", required_argument, 0, 'b'},
            {"contrast",   required_argument, 0, 'k'},
            {"help",       no_argument,       0, 'h'},
            {0, 0,                            0, 0}};

    while (1) {
        opt = getopt_long(argc, argv, "V:B::o:h", long_options, NULL);
        if (opt == -1) {
            break;
        }

        switch (opt) {
            case 'V':
                if (!stringToInt(optarg, &V_option)) {
                    fprintf(stderr, "Could not pass argument for option -V: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'B':
                if (optarg) {
                    if (!stringToInt(optarg, &B_option)) {
                        fprintf(stderr, "Could not pass argument for option -B: %s\n", optarg);
                        return EXIT_FAILURE;
                    }
                    if (!B_option) {
                        fprintf(stderr, "Option -B can not be zero.\n");
                        return EXIT_FAILURE;
                    }
                } else {
                    B_option = 1;
                }
                break;
            case 'o':
                output_filename = optarg;
                break;

            case 'h':
                printHelp();
                return EXIT_SUCCESS;

            case 'c':
                if (!parseAndStoreCoeffs(optarg, coeffs)) {
                    fprintf(stderr, "Error: Could not parse coefficients.\n");
                    return EXIT_FAILURE;
                }
                break;

            case 'b':
                if (!stringToInt(optarg, &brightness)) {
                    fprintf(stderr, "Could not pass argument for option --brightness: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;

            case 'k':
                if (!stringToInt(optarg, &tmp_contrast)) {
                    fprintf(stderr, "Could not pass argument for option --contrast: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                contrast = (float) tmp_contrast;
                break;

            case '?':
                fprintf(stderr, "Error parsing options\n");
                return EXIT_FAILURE;
        }
    }

    // checks if only one positional argument exists
    if (optind == argc - 1) {
        input_filename = argv[optind];
    } else if (optind < argc - 1) {
        fprintf(stderr, "Too many positional arguments given.\n");
        return EXIT_FAILURE;
    } else {
        fprintf(stderr, "No input file specified.\n");
        return EXIT_FAILURE;
    }

    if (!checkParams(V_option, B_option, input_filename, output_filename, coeffs[0], coeffs[1], coeffs[2], brightness,
                     contrast)) {
        return EXIT_FAILURE;
    }

    // Read input image
    PPMImage *input_image = readPPM(input_filename);
    if (!input_image) {
        fprintf(stderr, "readPPM returned NULL. Reading input image not possible.\n");
        return EXIT_FAILURE;
    }

    // check overflow for width * height
    size_t wh;
    if (__builtin_umull_overflow(input_image->width, input_image->height, &wh)) {
        fprintf(stderr, "image is too large. Overflow happened.\n");
        freePPM(input_image);
        return EXIT_FAILURE;
    }

    // Allocate memory for new image
    uint8_t *new_pixels = malloc(wh);
    if (!new_pixels) {
        fprintf(stderr, "malloc for new_pixels returned NULL\n");
        freePPM(input_image);
        return EXIT_FAILURE;
    }

    // Start Image Conversion
    int counter = 0;
    int exec_res;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    switch (V_option) {
        case 2:
            do {
                exec_res = brightness_contrast_V2(
                        input_image->data, input_image->width, input_image->height,
                        coeffs[0], coeffs[1], coeffs[2],
                        brightness, contrast, new_pixels);
                counter++;
            } while (counter < B_option);
            break;
        case 1:
            do {
                exec_res = brightness_contrast_V1(
                        input_image->data, input_image->width, input_image->height,
                        coeffs[0], coeffs[1], coeffs[2],
                        brightness, contrast, new_pixels);
                counter++;
            } while (counter < B_option);
            break;

        case 0:
        default:
            do {
                exec_res = brightness_contrast_V0(
                        input_image->data, input_image->width, input_image->height,
                        coeffs[0], coeffs[1], coeffs[2],
                        brightness, contrast, new_pixels);
                counter++;
            } while (counter < B_option);
            break;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (!exec_res) {
        freePPM(input_image);
        free(new_pixels);
        fprintf(stderr, "Execution failed with version %d\n", V_option);
        return EXIT_FAILURE;
    }

    if (B_option) {
        double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
        double average_time = time / B_option;
        printf("The implementation takes %f seconds for %d iteration(s) of version %d of the implementation. Average: %f seconds (excluding reading and writing the file)\n",
               time, B_option, V_option, average_time);
    }

    // Write PGM file
    int retPGM = writePGM(output_filename, new_pixels, input_image->width, input_image->height);

    // Free remaining ressources 
    freePPM(input_image);
    free(new_pixels);

    if(!retPGM) {
        // return exit failure if image could not be written
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}