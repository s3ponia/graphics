#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "../include/defines.h"
#include "../include/picture.h"
#include "../include/utility.h"
#include "../include/color_space.h"

unsigned char do_correction(unsigned char val, long offset, float factor) {
    return round(fmaxf(0.f, fminf(255.f, fmaxf(0.f, (float) (val - offset)) * factor)));
}

unsigned char auto_correction(unsigned char val, int min_val, unsigned char max_val) {
    return round(fmaxf(0.f, fminf(255.f, fmaxf(0.f, (float) (val - min_val)) * 255.f / (float) (max_val - min_val))));
}

void rgb_correct(unsigned char *r, unsigned char *g, unsigned char *b, long offset, float factor) {
    *r = do_correction(*r, offset, factor);
    *g = do_correction(*g, offset, factor);
    *b = do_correction(*b, offset, factor);
}

void YCbCr601_correct(unsigned char *y, unsigned char *cb, unsigned char *cr,
                      long offset, float factor) {
    *y = do_correction(*y, offset, factor);
}

void rgb_auto_correct(unsigned char *r, unsigned char *g, unsigned char *b, unsigned char max_val, unsigned min_val) {
    *r = auto_correction(*r, min_val, max_val);
    *g = auto_correction(*g, min_val, max_val);
    *b = auto_correction(*b, min_val, max_val);
}

void YCbCr601_auto_correct(unsigned char *y, unsigned char *cb, unsigned char *cr,
                           unsigned char min_val, unsigned max_val) {
    *y = auto_correction(*y, min_val, max_val);
}

typedef void (*correct_policy)(unsigned char *s1, unsigned char *s2, unsigned char *s3, long offset,
                               float factor);

typedef void (*auto_correct_policy)(unsigned char *s1, unsigned char *s2, unsigned char *s3,
                                    unsigned char min_val, unsigned max_val);

void correct_3(picture *pic, long offset, float factor, correct_policy policy) {
    assert(pic->type == P6);
    for (size_t y = 0; y < pic->height; ++y) {
        for (int x = 0; x < pic->width; ++x) {
            unsigned char *c = get_data(pic, x, y);
            policy(c, c + 1, c + 2, offset, factor);
        }
    }
}

void auto_correct_3(picture *pic, auto_correct_policy policy, unsigned char min, unsigned char max) {
    assert(pic->type == P6);
    for (size_t y = 0; y < pic->height; ++y) {
        for (int x = 0; x < pic->width; ++x) {
            unsigned char *c = get_data(pic, x, y);
            policy(c, c + 1, c + 2, min, max);
        }
    }
}

void correct(picture *pic, long offset, float factor) {
    for (size_t i = 0; i < picture_size(pic); ++i) {
        pic->data[i] = do_correction(pic->data[i], offset, factor);
    }
}

void auto_correct(picture *pic, auto_correct_policy policy, unsigned char min, unsigned char max) {
    for (size_t i = 0; i < picture_size(pic); ++i) {
        pic->data[i] = auto_correction(pic->data[i], min, max);
    }
}

void rgb_find_min_max(picture *pic, unsigned char *min, unsigned char *max) {
    assert(pic != NULL && min != NULL && max != NULL);
    *min = 255;
    *max = 0;
    for (size_t i = 0; i < picture_size(pic); ++i) {
        *max = fmax(pic->data[i], *max);
        *min = fmin(pic->data[i], *min);
    }
}

void rgb_find_min_max_with_skip(picture *pic, unsigned char *min, unsigned char *max) {
    assert(min != NULL && max != NULL);
    int cnt[256] = {};
    for (size_t i = 0; i < picture_size(pic); ++i) {
        ++cnt[pic->data[i]];
    }

    *min = 255;
    *max = 0;

    const int skip = 0.0039 * picture_size(pic);
    int count = 0;
    for (int i = 0; i < sizeof(cnt) / sizeof(cnt[0]); ++i) {
        if (count + cnt[i] < skip) {
            count += cnt[i];
        } else {
            *min = i;
            break;
        }
    }

    count = 0;

    for (int i = sizeof(cnt) / sizeof(cnt[0]) - 1; i >= 0; --i) {
        if (count + cnt[i] < skip) {
            count += cnt[i];
        } else {
            *max = i;
            break;
        }
    }
}

void YCbCr601_find_min_max(picture *pic, unsigned char *min, unsigned char *max) {
    assert(pic->type == P6 && min != NULL && max != NULL);
    *min = 255;
    *max = 0;
    for (size_t y = 0; y < pic->height; ++y) {
        for (size_t x = 0; x < pic->width; ++x) {
            *max = fmax(*get_data(pic, x, y), *max);
            *min = fmin(*get_data(pic, x, y), *min);
        }
    }
}

void YCbCr601_find_min_max_with_skip(picture *pic,
                                     unsigned char *min, unsigned char *max) {
    assert(pic->type == P6 && min != NULL && max != NULL);
    int cnt[256] = {};
    for (size_t y = 0; y < pic->height; ++y) {
        for (size_t x = 0; x < pic->width; ++x) {
            ++cnt[*get_data(pic, x, y)];
        }
    }

    *min = 255;
    *max = 0;

    const int skip = 0.0039 * picture_size(pic);
    int count = 0;
    for (size_t i = 0; i < sizeof(cnt) / sizeof(cnt[0]); ++i) {
        if (count + cnt[i] < skip) {
            count += cnt[i];
        } else {
            *min = i;
            break;
        }
    }

    count = 0;

    for (int i = sizeof(cnt) / sizeof(cnt[0]) - 1; i >= 0; --i) {
        if (count + cnt[i] < skip) {
            count += cnt[i];
        } else {
            *max = i;
            break;
        }
    }
}

static void YCbCr601_to_rgb(picture *pic) {
    assert(pic->type == P6);

    for (size_t y = 0; y < pic->height; ++y) {
        for (size_t x = 0; x < pic->width; ++x) {
            unsigned char *c = get_data(pic, x, y);
            YCbCr_601_to_rgb_pixel(c, c + 1, c + 2);
        }
    }
}

static void rgb_to_YCbCr601(picture *pic) {
    assert(pic->type == P6);

    for (size_t y = 0; y < pic->height; ++y) {
        for (size_t x = 0; x < pic->width; ++x) {
            unsigned char *c = get_data(pic, x, y);
            YCbCr_601_from_rgb_pixel(c, c + 1, c + 2);
        }
    }
}

int task5(int argc, char *argv[]) {
    if (argc != 4 && argc != 6) {
        fprintf(stderr,
                "usage:\n%s  <имя_входного_файла> <имя_выходного_файла> "
                "<преобразование> [<смещение> <множитель>]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    unsigned transformation_type;
    READ_INT(transformation_type, argv[3], {
        perror("error in parsing <преобразование>.");
        return EXIT_FAILURE;
    }, strtol);

    long offset;
    float factor;

    if (argc == 6) {
        READ_INT(offset, argv[4], {
            perror("error in parsing <смещение>.");
            return EXIT_FAILURE;
        }, strtol);
        if (offset < -255 || offset > 255) {
            fprintf(stderr, "offset must be in [0;255]");
            return EXIT_FAILURE;
        }

        READ_FLOAT(factor, argv[5], {
            perror("error in parsing <множитель>.");
            return EXIT_FAILURE;
        }, strtof);
        if (factor < 0 || factor > 255) {
            fprintf(stderr, "offset must be in [0;255]");
            return EXIT_FAILURE;
        }
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

    struct picture *const picture = malloc(sizeof(struct picture) + size * sizeof(char));

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

    switch (transformation_type) {
        case 0: {
            correct(picture, offset, factor);
            break;
        }
        case 1: {
            rgb_to_YCbCr601(picture);

            if (picture->type == P5) {
                fprintf(stderr, "picture should have type P6.");
                goto error;
            }

            correct_3(picture, offset, factor, YCbCr601_correct);

            YCbCr601_to_rgb(picture);
            break;
        }

        case 2: {
            unsigned char max;
            unsigned char min;
            rgb_find_min_max(picture, &min, &max);

            auto_correct(picture, rgb_auto_correct, min, max);

            printf("%d %f", min, 255.f / (max - min));
            break;
        }

        case 3: {
            unsigned char max;
            unsigned char min;

            if (picture->type == P5) {
                fprintf(stderr, "picture should have type P6.");
                goto error;
            }

            rgb_to_YCbCr601(picture);
            YCbCr601_find_min_max(picture, &min, &max);

            auto_correct_3(picture, YCbCr601_auto_correct, min, max);

            YCbCr601_to_rgb(picture);
            printf("%d %f", min, 255.f / (max - min));
            break;
        }

        case 4: {
            unsigned char max;
            unsigned char min;

            rgb_find_min_max_with_skip(picture, &min, &max);

            auto_correct(picture, rgb_auto_correct, min, max);

            printf("%d %f", min, 255.f / (max - min));
            break;
        }

        case 5: {
            unsigned char max;
            unsigned char min;

            if (picture->type == P5) {
                fprintf(stderr, "picture should have type P6.");
                goto error;
            }

            rgb_to_YCbCr601(picture);
            YCbCr601_find_min_max_with_skip(picture, &min, &max);

            auto_correct_3(picture, YCbCr601_auto_correct, min, max);

            YCbCr601_to_rgb(picture);
            printf("%d %f", min, 255.f / (max - min));
            break;
        }

        default: {
            fprintf(stderr, "Unhandled transformation type.\n");
            goto error;
        }

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
    return task5(argc, argv);
}
