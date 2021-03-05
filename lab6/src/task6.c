#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "../include/defines.h"
#include "../include/picture.h"
#include "../include/utility.h"

picture *nearest_neighbourd(const picture *pic, int width, int height) {
    picture *result = malloc(sizeof(picture) + (pic->type == P5 ? 1 : 3) * width * height);
    if (!result) {
        return NULL;
    }
    result->height = height;
    result->width = width;
    result->type = pic->type;
    result->max_color = pic->max_color;
    result->pixel_size = pic->pixel_size;

    int x_ratio = (int) ((pic->width << 16) / width) + 1;
    int y_ratio = (int) ((pic->height << 16) / height) + 1;
    int x2, y2;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int i = 0; i < (pic->type == P5 ? 1 : 3); ++i) {
                x2 = ((x * x_ratio) >> 16);
                y2 = ((y * y_ratio) >> 16);
                get_data(result, x, y)[i] = const_get_data(pic, x2, y2)[i];
            }
        }
    }
    return result;
}

unsigned char bilinear_approx(const float dx,
                              const float dy,
                              const unsigned char c00,
                              const unsigned char c10,
                              const unsigned char c01,
                              const unsigned char c11) {
    float a = c00 * (1 - dx) + c10 * dx;
    float b = c01 * (1 - dx) + c11 * dx;
    return a * (1 - dy) + b * dy;
}

picture *bilinear_interpolation(const picture *pic, int width, int height, float gamma) {
    picture *result = malloc(sizeof(picture) + (pic->type == P5 ? 1 : 3) * width * height);
    if (!result) {
        return NULL;
    }
    result->height = height;
    result->width = width;
    result->type = pic->type;
    result->max_color = pic->max_color;
    result->pixel_size = pic->pixel_size;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double gx = x / (double) (width) * (pic->width - 1);
            double gy = y / (double) (height) * (pic->height - 1);

            const int x_ = round(gx);
            const int y_ = round(gy);

            const int x_2 = x_ + 1 >= pic->width ? x_ : x_ + 1;
            const int y_2 = y_ + 1 >= pic->height ? y_ : y_ + 1;
            const unsigned char *pixel_c00 = const_get_data(pic, x_, y_);
            const unsigned char *pixel_c10 = const_get_data(pic, x_2, y_);
            const unsigned char *pixel_c01 = const_get_data(pic, x_, y_2);
            const unsigned char *pixel_c11 = const_get_data(pic, x_2, y_2);

            for (int i = 0; i < (pic->type == P5 ? 1 : 3); ++i) {
                get_data(result, x, y)[i] = gamma_correction(bilinear_approx(gx - x_,
                                                                             gy - y_,
                                                                             gamma_back_correction(pixel_c00[i], gamma),
                                                                             gamma_back_correction(pixel_c10[i], gamma),
                                                                             gamma_back_correction(pixel_c01[i], gamma),
                                                                             gamma_back_correction(pixel_c11[i], gamma)
                ), gamma);
            }
        }
    }

    return result;
}

double sinc(double x) {
    x = (x * M_PI);
    if (x < 0.01 && x > -0.01)
        return 1.0 + x * x * (-1.0 / 6.0 + x * x * 1.0 / 120.0);
    return sin(x) / x;
}

double lanczos3_kernel(double x) {
    if (fabs(x) < 3) {
        return sinc(x) * sinc(x / 3);
    } else {
        return 0.0;
    }
}

picture *lanczos_3(const picture *pic, int width, int height, float gamma) {
    picture *result = malloc(sizeof(picture) + (pic->type == P5 ? 1 : 3) * width * height);
    if (!result) {
        return NULL;
    }
    result->height = height;
    result->width = width;
    result->type = pic->type;
    result->max_color = pic->max_color;
    result->pixel_size = pic->pixel_size;

    const int lanczos_size = 3;

    const float width_ratio =
            (float) pic->width / width;
    const float height_ratio =
            (float) pic->height / height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int old_x = width_ratio * x;
            const int old_y = height_ratio * y;

            for (int i = 0; i < (pic->type == P5 ? 1 : 3); ++i) {
                float sum = 0;
                float weight = 0;
                for (int x_ = old_x - lanczos_size + 1; x_ <= old_x + lanczos_size; ++x_) {
                    for (int y_ = old_y - lanczos_size + 1; y_ <= old_y + lanczos_size; ++y_) {
                        if (x_ >= 0 && x_ < pic->width && y_ >= 0 && y_ < pic->height) {
                            const double lanc_coef =
                                    lanczos3_kernel(height_ratio * y - y_) * lanczos3_kernel(width_ratio * x - x_);
                            sum += gamma_back_correction(const_get_data(pic, x_, y_)[i], gamma) / 255. * lanc_coef;
                            weight += lanc_coef;
                        }
                    }
                }
                const float pix_res = fmaxf(0.f, fminf(1.f, sum / weight));
                get_data(result, x, y)[i] = gamma_correction(pix_res * 255.f, gamma);
            }
        }
    }

    return result;
}

double bcsplines_kernel(double x, double b, double c) {
    if (fabsf(x) < 1) {
        return ((12 - 9 * b - 6 * c) * pow(fabs(x), 3) + (-18 + 12 * b + 6 * c) * pow(fabs(x), 2) + (6 - 2 * b)) / 6.;
    } else if (fabsf(x) < 2) {
        return ((-b - 6 * c) * pow(fabs(x), 3) + (6 * b + 30 * c) * pow(fabs(x), 2) + (-12 * b - 48 * c) * fabs(x) +
               (8 * b + 24 * c)) / 6.;
    } else {
        return 0.0;
    }
}

picture *bcsplines(const picture *pic, int width, int height, float gamma, float b, float c) {
    picture *result = malloc(sizeof(picture) + (pic->type == P5 ? 1 : 3) * width * height);
    if (!result) {
        return NULL;
    }
    result->height = height;
    result->width = width;
    result->type = pic->type;
    result->max_color = pic->max_color;
    result->pixel_size = pic->pixel_size;

    const int radius = 2;

    const float width_ratio =
            (float) pic->width / width;
    const float height_ratio =
            (float) pic->height / height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int old_x = width_ratio * x;
            const int old_y = height_ratio * y;


            for (int i = 0; i < (pic->type == P5 ? 1 : 3); ++i) {
                float sum = 0;
                float weight = 0;
                for (int x_ = old_x - radius - 1; x_ <= old_x + radius; ++x_) {
                    for (int y_ = old_y - radius - 1; y_ <= old_y + radius; ++y_) {
                        if (x_ >= 0 && x_ < pic->width && y_ >= 0 && y_ < pic->height) {
                            const double bcs_1 = bcsplines_kernel(height_ratio * y - y_, b, c);
                            const double bcs_2 = bcsplines_kernel(width_ratio * x - x_, b, c);
                            const double bcs_coef = bcs_1 * bcs_2;
                            sum += gamma_back_correction(const_get_data(pic, x_, y_)[i], gamma) / 255. * bcs_coef;
                            weight += bcs_coef;
                        }
                    }
                }
                const float pix_res = fmaxf(0.f, fminf(1.f, sum / weight));
                get_data(result, x, y)[i] = gamma_correction(pix_res * 255.f, gamma);
            }
        }
    }

    return result;
}

int task2(int argc, char *argv[]) {
    if (argc != 9 && argc != 11) {
        fprintf(stderr,
                "usage:\n%s <input> <output> <width> <height> <dx> <dy> <gamma> <type> [<B> <C>]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    unsigned long width;
    READ_INT(width, argv[3], {
        perror("error in parsing <width>.");
        return EXIT_FAILURE;
    }, strtoul);

    double height;
    READ_FLOAT(height, argv[4], {
        perror("error in parsing <height>.");
        return EXIT_FAILURE;
    }, strtod);

    point center = {};
    READ_INT(center.x, argv[5], {
        perror("error in parsing <dx>.");
        return EXIT_FAILURE;
    }, strtoul);
    READ_INT(center.y, argv[6], {
        perror("error in parsing <dy>.");
        return EXIT_FAILURE;
    }, strtoul);

    double gamma = 2.2;
    READ_FLOAT(gamma, argv[7], {
        perror("error in parsing <гамма>.");
        return EXIT_FAILURE;
    }, strtod);

    int type = 0;
    READ_INT(type, argv[8], {
        perror("error in parsing <type>.");
        return EXIT_FAILURE;
    }, strtol);


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

    switch (type) {
        case 0: {
            struct picture *temp = nearest_neighbourd(picture, width, height);
            if (temp == NULL) {
                perror("can't allocate memory for result image.");
                goto error;
            }
            free(picture);
            picture = temp;
            break;
        }
        case 1: {
            struct picture *temp = bilinear_interpolation(picture, width, height, gamma);
            if (temp == NULL) {
                perror("can't allocate memory for result image.");
                goto error;
            }
            free(picture);
            picture = temp;
            break;
        }
        case 2: {
            struct picture *temp = lanczos_3(picture, width, height, gamma);
            if (temp == NULL) {
                perror("can't allocate memory for result image.");
                goto error;
            }
            free(picture);
            picture = temp;
            break;
        }
        case 3: {
            float b = 0;
            float c = 0.5;
            if (argc == 11) {
                READ_FLOAT(b, argv[7], {
                    perror("error in parsing <b>.");
                    goto error;
                }, strtod);
                READ_FLOAT(c, argv[7], {
                    perror("error in parsing <c>.");
                    goto error;
                }, strtod);
            }
            struct picture *temp = bcsplines(picture, width, height, gamma, b, c);
            if (temp == NULL) {
                perror("can't allocate memory for result image.");
                goto error;
            }
            free(picture);
            picture = temp;
            break;
        }
        default: {
            fprintf(stderr, "Unhandled transform type.");
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
    return task2(argc, argv);
}