#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../include/utility.h"
#include "../include/defines.h"
#include "../include/picture.h"
#include "../include/color_space.h"

#define USAGE "usage:\n%s  -f <from_color_space> -t <to_color_space>" \
"  -i <count> <input_file_name> -o <count> <output_file_name>\n"

const to_rgb_pixel_func to_rgb_funcs[] = {
        [RGB] = noop,
        [HSL] = hsl_to_rgb_pixel,
        [HSV] = hsv_to_rgb_pixel,
        [YCbCr_601] = YCbCr_601_to_rgb_pixel,
        [YCbCr_709] = YCbCr_709_to_rgb_pixel,
        [YCoCg] = YCoCg_to_rgb_pixel,
        [CMY] = CMY_to_rgb_pixel
};

const from_rgb_pixel_func from_rgb_funcs[] = {
        [RGB] = noop,
        [HSL] = hsl_from_rgb_pixel,
        [HSV] = hsv_from_rgb_pixel,
        [YCbCr_601] = YCbCr_601_from_rgb_pixel,
        [YCbCr_709] = YCbCr_709_from_rgb_pixel,
        [YCoCg] = YCoCg_from_rgb_pixel,
        [CMY] = CMY_from_rgb_pixel
};

static void to_rgb(color_sources sources, to_rgb_pixel_func func) {
    assert(sources.sources[0]->type == P5);
    assert(sources.sources[1]->type == P5);
    assert(sources.sources[2]->type == P5);
    assert(sources.sources[0]->width == sources.sources[1]->width &&
           sources.sources[1]->width == sources.sources[2]->width &&
           sources.sources[0]->width == sources.sources[2]->width &&
           sources.sources[0]->height == sources.sources[1]->height &&
           sources.sources[1]->height == sources.sources[2]->height &&
           sources.sources[0]->height == sources.sources[2]->height
    );

    for (size_t i = 0; i < sources.sources[0]->width * sources.sources[0]->height; ++i) {
        func(&sources.sources[0]->data[i],
             &sources.sources[1]->data[i],
             &sources.sources[2]->data[i]);
    }
}

static void from_rgb(color_sources sources, from_rgb_pixel_func func) {
    assert(sources.sources[0]->type == P5);
    assert(sources.sources[1]->type == P5);
    assert(sources.sources[2]->type == P5);
    assert(sources.sources[0]->width == sources.sources[1]->width &&
           sources.sources[1]->width == sources.sources[2]->width &&
           sources.sources[0]->width == sources.sources[2]->width &&
           sources.sources[0]->height == sources.sources[1]->height &&
           sources.sources[1]->height == sources.sources[2]->height &&
           sources.sources[0]->height == sources.sources[2]->height
    );

    for (size_t i = 0; i < sources.sources[0]->width * sources.sources[0]->height; ++i) {
        func(&sources.sources[0]->data[i],
             &sources.sources[1]->data[i],
             &sources.sources[2]->data[i]);
    }
}

color_space from_string(char *s) {
    assert(s != NULL);
    if (!strncmp(s, "RGB", 3))
        return RGB;
    else if (!strncmp(s, "HSL", 3))
        return HSL;
    else if (!strncmp(s, "HSV", 3))
        return HSV;
    else if (!strncmp(s, "YCbCr_601", sizeof("YCbCr_601") - 1))
        return YCbCr_601;
    else if (!strncmp(s, "YCbCr_709", sizeof("YCbCr_709") - 1))
        return YCbCr_709;
    else if (!strncmp(s, "YCoCg", sizeof("YCoCg") - 1))
        return YCoCg;
    else if (!strncmp(s, "CMY", sizeof("CMY") - 1))
        return CMY;
    assert(true);
}

int main(int argc, char *argv[]) {
    if (argc != 11) {
        fprintf(stderr, USAGE, argv[0]);
        return EXIT_FAILURE;
    }

    ++argv;
    color_space from = -1;
    color_space to = -1;
    int input_file_count = 0;
    int output_file_count = 0;
    char **sv = argv;
    picture *input_pictures[3] = {};
    picture *input_picture = NULL;
    filename output_file_names[3] = {};
    char *datas[3] = {};
    FILE *input = NULL;
    while (*argv != NULL) {
        char *arg_it = *argv;
        if (*arg_it != '-') {
            fprintf(stderr, USAGE,
                    sv[0]);
            goto clear;
        }

        ++argv;
        if (!argv) {
            fprintf(stderr, USAGE,
                    sv[0]);
            goto clear;
        }
        switch (arg_it[1]) {
            case 'f':
                from = from_string(*argv);
                break;
            case 't':
                to = from_string(*argv);
                break;
            case 'i':
                READ_INT(input_file_count, *argv, {
                    perror("error in parsing <count>.");
                    goto clear;
                }, strtol);
                if (input_file_count != 1 && input_file_count != 3) {
                    fprintf(stderr, USAGE,
                            sv[0]);
                    goto clear;
                }
                filename file_names[3] = {};
                if (input_file_count == 1) {
                    ++argv;
                    if (!argv) {
                        fprintf(stderr, USAGE,
                                sv[0]);
                        goto clear;
                    }
                    memcpy(file_names[0].data, *argv, strlen(*argv) + 1);
                }

                if (input_file_count == 3) {
                    ++argv;
                    if (!argv) {
                        fprintf(stderr, USAGE,
                                sv[0]);
                        goto clear;
                    }
                    char *pattern = *argv;
                    char *end = pattern + strlen(pattern);
                    char *dot = end;
                    while (*dot != '.' && dot != pattern) {
                        --dot;
                    }
                    if (dot == pattern) {
                        fprintf(stderr, USAGE,
                                sv[0]);
                        goto clear;
                    }

                    size_t body_size = dot - pattern;
                    memcpy(file_names[0].data, pattern, dot - pattern);
                    memcpy(file_names[1].data, pattern, dot - pattern);
                    memcpy(file_names[2].data, pattern, dot - pattern);

                    memcpy(file_names[0].data + body_size, "_1", 2);
                    memcpy(file_names[1].data + body_size, "_2", 2);
                    memcpy(file_names[2].data + body_size, "_3", 2);

                    memcpy(file_names[0].data + body_size + 2, dot, end - dot + 1);
                    memcpy(file_names[1].data + body_size + 2, dot, end - dot + 1);
                    memcpy(file_names[2].data + body_size + 2, dot, end - dot + 1);
                }

                for (int i = 0; i < input_file_count; ++i) {
                    if (input != NULL)
                        fclose(input);
                    input = fopen(file_names[i].data, "rb");
                    if (!input) {
                        goto clear;
                    }
                    size_t sz;
                    int ret;
                    if ((ret = read_all(input, &datas[i], &sz))) {
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
                        goto clear;
                    }
                    input_pictures[i] = malloc(sizeof(struct picture) + sz * sizeof(char));
                    if (!input_pictures[i]) {
                        fprintf(stderr, "NOMEM: can't allocate memory for picture.");
                        goto clear;
                    }

                    const char *end = NULL;
                    if ((ret = read_header(datas[i], sz, &input_pictures[i]->type, &input_pictures[i]->width,
                                           &input_pictures[i]->height,
                                           &input_pictures[i]->max_color,
                                           &input_pictures[i]->pixel_size, &end)) != SUCCESS) {
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
                        goto clear;
                    }

                    memcpy(input_pictures[i]->data, end,
                           input_pictures[i]->height * input_pictures[i]->width * input_pictures[i]->pixel_size *
                           (input_pictures[i]->type == P5 ? 1 : 3));
                }
                break;

            case 'o':
                READ_INT(output_file_count, *argv, {
                    perror("error in parsing <count>.");
                    goto clear;
                }, strtol);

                if (output_file_count != 1 && output_file_count != 3) {
                    fprintf(stderr, USAGE,
                            sv[0]);
                    goto clear;
                }

                if (output_file_count == 1) {
                    ++argv;
                    if (!argv) {
                        fprintf(stderr, USAGE,
                                sv[0]);
                        goto clear;
                    }
                    memcpy(output_file_names[0].data, *argv, strlen(*argv) + 1);
                }

                if (output_file_count == 3) {
                    ++argv;
                    if (!argv) {
                        fprintf(stderr, USAGE,
                                sv[0]);
                        goto clear;
                    }
                    char *pattern = *argv;
                    char *end = pattern + strlen(pattern);
                    char *dot = end;
                    while (*dot != '.' && dot != pattern) {
                        --dot;
                    }
                    if (dot == pattern) {
                        fprintf(stderr, USAGE,
                                sv[0]);
                        goto clear;
                    }

                    size_t body_size = dot - pattern;
                    memcpy(output_file_names[0].data, pattern, dot - pattern);
                    memcpy(output_file_names[1].data, pattern, dot - pattern);
                    memcpy(output_file_names[2].data, pattern, dot - pattern);

                    memcpy(output_file_names[0].data + body_size, "_1", 2);
                    memcpy(output_file_names[1].data + body_size, "_2", 2);
                    memcpy(output_file_names[2].data + body_size, "_3", 2);

                    memcpy(output_file_names[0].data + body_size + 2, dot, end - dot + 1);
                    memcpy(output_file_names[1].data + body_size + 2, dot, end - dot + 1);
                    memcpy(output_file_names[2].data + body_size + 2, dot, end - dot + 1);
                }
                break;
            default:
                fprintf(stderr, USAGE,
                        sv[0]);
                return EXIT_FAILURE;
        }

        ++argv;
        continue;
        clear:
        free(input_pictures[2]);
        free(input_pictures[1]);
        free(input_pictures[0]);
        free(datas[2]);
        free(datas[1]);
        free(datas[0]);
        if (input != NULL)
            fclose(input);

        return EXIT_FAILURE;
    }
    free(datas[2]);
    free(datas[1]);
    free(datas[0]);

    if (input_file_count == 1) {
        input_picture = input_pictures[0];
        const size_t sz = input_picture->width * input_picture->height;
        input_pictures[0] = malloc(sizeof(struct picture) + sz * sizeof(char));
        if (!input_pictures[0]) {
            goto error_clear;
        }
        input_pictures[1] = malloc(sizeof(struct picture) + sz * sizeof(char));
        if (!input_pictures[1]) {
            goto error_clear0;
        }
        input_pictures[2] = malloc(sizeof(struct picture) + sz * sizeof(char));
        if (!input_pictures[2]) {
            goto error_clear1;
        }

        for (int i = 0; i < 3; ++i) {
            input_pictures[i]->height = input_picture->height;
            input_pictures[i]->width = input_picture->width;
            input_pictures[i]->type = P5;
        }

        for (int i = 0; i < input_picture->width; ++i) {
            for (int j = 0; j < input_picture->height; ++j) {
                const char *arr = get_data(input_picture, i, j);
                *get_data(input_pictures[0], i, j) = arr[0];
                *get_data(input_pictures[1], i, j) = arr[1];
                *get_data(input_pictures[2], i, j) = arr[2];
            }
        }
    }

    color_sources sources = {
            .sources = {
                    [0] = input_pictures[0],
                    [1] = input_pictures[1],
                    [2] = input_pictures[2]
            }
    };

    to_rgb(sources, to_rgb_funcs[from]);
    from_rgb(sources, to_rgb_funcs[to]);

    if (output_file_count == 1) {
        if (input != NULL)
            fclose(input);
        input = fopen(output_file_names[0].data, "wb");
        if (!input) {
            fprintf(stderr, "Error in opening file %s for output", output_file_names[0].data);
            goto error_clear2;
        }
        for (size_t i = 0; i < input_picture->height * input_picture->width; ++i) {
            unsigned char *arr = &input_picture->data[i];
            arr[0] = sources.sources[0]->data[i];
            arr[1] = sources.sources[1]->data[i];
            arr[2] = sources.sources[2]->data[i];
        }
        int ret;
        if ((ret = save_picture(input_picture, input)) != SUCCESS) {
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
            goto error_clear2;
        }
    } else if (output_file_count == 3) {
        for (int i = 0; i < output_file_count; ++i) {
            if (input != NULL)
                fclose(input);
            input = fopen(output_file_names[i].data, "wb");
            if (!input) {
                fprintf(stderr, "Error in opening file %s for output", output_file_names[0].data);
                goto error_clear2;
            }
            int ret;
            if ((ret = save_picture(sources.sources[i], input)) != SUCCESS) {
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
                goto error_clear2;
            }
        }
    }

    free(input_pictures[2]);
    free(input_pictures[1]);
    free(input_pictures[0]);
    free(input_picture);
    if (input != NULL)
        fclose(input);
    return EXIT_SUCCESS;
    error_clear2:
    free(input_pictures[2]);
    error_clear1:
    free(input_pictures[1]);
    error_clear0:
    free(input_pictures[0]);
    error_clear:
    free(input_picture);
    if (input != NULL)
        fclose(input);
    return EXIT_FAILURE;
}