#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "../include/defines.h"
#include "../include/utility.h"
#include "../include/dithering.h"
#include "../include/picture.h"

int choose_dithering(unsigned type, dpicture *pic, unsigned bits, double gamma) {
    static const void *dither_functions[] = {
            [NO_DITHERING] = pixel_no_dithering,
            [ORDERED_DITHERING] = pixel_ordered,
            [RANDOM_DITHERING] = pixel_random,
            [FLOYD_STEINBERG_DITHERING] = pixel_floyd,
            [JJN_DITHERING] = pixel_jjn,
            [SIERA_DITHERING] = pixel_siera,
            [ATKINSON_DITHERING] = pixel_atkinson,
            [HALFTONE] = pixel_halftone
    };
    srand(time(NULL));
    assert(pic != NULL && bits > 0 && bits <= 8 && gamma >= 0);
    if (type >= 8) {
        errno = EINVAL;
        return EINVAL;
    }
    if (type >= 3 && type != HALFTONE) {
        error_diffusion(pic, bits, dither_functions[type], gamma);
    } else {
        ordered_dither(pic, bits, gamma, dither_functions[type]);
    }
    return 0;
}

int task3(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr,
                "usage:\n%s <имя_входного_файла> <имя_выходного_файла>"
                " <градиент> <дизеринг> <битность> <гамма>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    unsigned long gradient;
    READ_INT(gradient, argv[3], {
        perror("error in parsing <градиент>.");
        return EXIT_FAILURE;
    }, strtoul);

    if (gradient != 0 && gradient != 1) {
        fprintf(stderr, "<градиент> can be only 0 or 1.");
        return EXIT_FAILURE;
    }

    unsigned long dithering;
    READ_INT(dithering, argv[4], {
        perror("error in parsing <дизеринг>.");
        return EXIT_FAILURE;
    }, strtoul);
    if (dithering < 0 || dithering > 8) {
        fprintf(stderr, "<дизеринг> must be between 0 and 8.");
        return EXIT_FAILURE;
    }

    unsigned long bits;
    READ_INT(bits, argv[5], {
        perror("error in parsing <битность>.");
        return EXIT_FAILURE;
    }, strtoul);

    double gamma;
    READ_FLOAT(gamma, argv[6], {
        perror("error in parsing <гамма>.");
        return EXIT_FAILURE;
    }, strtod);

    if (fabs(gamma) < EPS) {
        gamma = 0.;
    }

    FILE *input_file = fopen(argv[1], "rb");
    if (input_file == NULL) {
        perror("can't open input file.");
        return EXIT_FAILURE;
    }

    FILE *output_file = fopen(argv[2], "wb");
    if (output_file == NULL) {
        fclose(input_file);
        perror("can't open output file.");
        return EXIT_FAILURE;
    }

    char *data;
    size_t size;
    int ret;

    if ((ret = read_all(input_file, &data, &size) != SUCCESS)) {
        const char *reason;
        switch (ret) {
            case NOMEM:
                reason = "no mem";
                break;
            case FILE_ERROR:
                reason = "file error";
                break;
            case OVERFLOW_ERROR:
                reason = "overflow";
                break;
            case LOGIC_ERROR:
                reason = "logic error";
                break;
            default:
                reason = "no reason";
                break;
        }
        fprintf(stderr, "%s: can't read all input file.", reason);
        fclose(input_file);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    struct picture *picture = malloc(sizeof(struct picture) + size * sizeof(char));

    if (picture == NULL) {
        fprintf(stderr, "NOMEM: can't allocate memory for picture.");
        fclose(input_file);
        fclose(output_file);
        free(data);
        return EXIT_FAILURE;
    }

    const char *end = NULL;
    if ((ret = read_header(data, size, &picture->type, &picture->width, &picture->height, &picture->max_color,
                                  &picture->pixel_size, &end)) != SUCCESS) {
        const char *reason;
        switch (ret) {
            case PARSE_ERROR:
                reason = "wrong file format";
                break;
            case LOGIC_ERROR:
                reason = "actual size doesn't match with size in header";
                break;
            default:
                reason = "no reason";
                break;
        }
        fprintf(stderr, "%s:can't parse file.", reason);
        fclose(input_file);
        fclose(output_file);
        free(data);
        free(picture);
        return EXIT_FAILURE;
    }

    memcpy(picture->data, end,
           picture->height * picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3));
    free(data);

    dpicture *dpic = malloc(picture->height * picture->width * sizeof(double));
    if (dpic == NULL) {
        perror("can't allocate memory for dpicture.");
        fclose(input_file);
        fclose(output_file);
        free(picture);
        return EXIT_FAILURE;
    }
    if (picture_to_dpicture(picture, dpic)) {
        perror("Error in converting picture to float.");
        fclose(input_file);
        fclose(output_file);
        free(picture);
        free(dpic);
        return EXIT_FAILURE;
    }

    if (gradient == 1) {
        fill_gradient(dpic);
    }

    if (choose_dithering(dithering, dpic, bits, gamma)) {
        perror("error in dithering.");
        fclose(input_file);
        fclose(output_file);
        free(picture);
        free(dpic);
        return EXIT_FAILURE;
    }

    if (dpicture_to_picture(dpic, picture)) {
        perror("Error in converting float picture to byte.");
        fclose(input_file);
        fclose(output_file);
        free(picture);
        free(dpic);
        return EXIT_FAILURE;
    }

    if ((ret = save_picture(picture, output_file)) != SUCCESS) {
        const char *reason;
        switch (ret) {
            case FILE_ERROR:
                reason = "io error";
                break;
            case LOGIC_ERROR:
                reason = "output file is null pointer";
                break;
            default:
                reason = "no reason";
                break;
        }
        fprintf(stderr, "%s:can't parse file.", reason);
        fclose(input_file);
        fclose(output_file);
        free(picture);
        free(dpic);
        return EXIT_FAILURE;
    }

    fclose(input_file);
    fclose(output_file);
    free(picture);
    free(dpic);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    return task3(argc, argv);
}