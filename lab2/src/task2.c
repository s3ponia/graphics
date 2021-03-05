#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "../include/defines.h"
#include "../include/picture.h"
#include "../include/utility.h"

int task2(int argc, char *argv[]) {
    if (argc != 9 && argc != 10) {
        fprintf(stderr,
                "usage:\n%s <имя_входного_файла> <имя_выходного_файла>"
                " <яркость_линии> <толщина_линии> <x_начальный>"
                " <y_начальный> <x_конечный> <y_конечный> <гамма>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    unsigned long line_brightness;
    READ_INT(line_brightness, argv[3], {
        perror("error in parsing <яркость_линии>.");
        return EXIT_FAILURE;
    }, strtoul);

    double line_width;
    READ_FLOAT(line_width, argv[4], {
        perror("error in parsing <толщина_линии>.");
        return EXIT_FAILURE;
    }, strtod);

    point start_point = {};
    READ_INT(start_point.x, argv[5], {
        perror("error in parsing <яркость_линии>.");
        return EXIT_FAILURE;
    }, strtoul);
    READ_INT(start_point.y, argv[6], {
        perror("error in parsing <y_начальный>.");
        return EXIT_FAILURE;
    }, strtoul);

    point end_point = {};
    READ_INT(end_point.x, argv[7], {
        perror("error in parsing <x_конечный>.");
        return EXIT_FAILURE;
    }, strtoul);
    READ_INT(end_point.y, argv[8], {
        perror("error in parsing <y_конечный>.");
        return EXIT_FAILURE;
    }, strtoul);

    double gamma = 2.2;
    if (argc == 10) {
        READ_FLOAT(gamma, argv[9], {
            perror("error in parsing <гамма>.");
            return EXIT_FAILURE;
        }, strtod);
    }

    FILE *input_file = fopen(argv[1], "rb");
    if (input_file == NULL) {
        perror("can't open input file.");
        return EXIT_FAILURE;
    }

    FILE *output_file = fopen(argv[2], "wb");
    if (output_file == NULL) {
        perror("can't open output file.");
        goto error_close_input_file;
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
        goto error_close_files;
    }

    struct picture *picture = malloc(sizeof(struct picture) + size * sizeof(char));

    if (picture == NULL) {
        fprintf(stderr, "NOMEM: can't allocate memory for picture.");
        free(data);
        goto error_close_files;
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
        free(data);
        goto error;
    }

    memcpy(picture->data, end,
           picture->height * picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3));
    free(data);

    if (start_point.x < 0 || start_point.x >= picture->width || start_point.y < 0 ||
        start_point.y >= picture->height || end_point.x < 0 ||
        end_point.x >= picture->width || end_point.y < 0 ||
        end_point.y >= picture->height) {
        fprintf(stderr, "start or end point is out of bounds.");
        goto error;
    }

    line_from_to(picture, start_point, end_point, line_brightness, gamma, line_width);

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
        goto error;
    }

    clear:
    fclose(input_file);
    fclose(output_file);
    free(picture);
    return EXIT_SUCCESS;

    error:
    free(picture);
    error_close_files:
    fclose(output_file);
    error_close_input_file:
    fclose(input_file);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    return task2(argc, argv);
}