#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#include "../include/dithering.h"
#include "../include/picture.h"
#include "../include/utility.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) > (b) ? (b) : (a))
#define HIGH_BITS(type, int, num) ((int) & (~(uintmax_t) 0 << (sizeof(type) * CHAR_BIT - (num))))
#define ARRAY_INDEX(x, y, w) ((x) + (y) * (w))

static unsigned char repeat_high_bits(float a, unsigned bits) {
    if (a < 0) {
        a = 0;
    } else if (a > 255) {
        a = 255;
    }
    unsigned char temp = HIGH_BITS(unsigned char, (unsigned char) a, bits);
    unsigned char result = temp;
    for (unsigned i = 1; i < CHAR_BIT / bits + (bits & 1u); ++i) {
        result |= (unsigned char) (temp >> (i * bits));
    }
    return result;
}

int fill_gradient(struct dpicture *p) {
    if (p == NULL || p->type != P5) {
        errno = EINVAL;
        return EINVAL;
    }

    for (size_t j = 0; j < p->height; ++j) {
        for (size_t i = 0; i < p->width; ++i) {
            size_t index = i + j * p->width;
            p->data[index] = (float) i / (p->width - 1);
        }
    }

    return 0;
}

static float unit_gamma_correction(float val, float gamma) {
    float ans, corrected_col = val;
    bool neg = false;
    if (val < 0) {
        neg = true;
        corrected_col = -val;
    }
    if (gamma == 0) {
        if (corrected_col <= 0.04045) ans = corrected_col / 12.92;
        else ans = pow((corrected_col + 0.055) / 1.055, 2.4);
    } else ans = pow(corrected_col, gamma);
    if (neg) return -ans;
    return ans;
}

static float
scaling_coeff(const float old_col, const unsigned char num_bits, const float gamma, pixel_ordered_dithering dith_func) {
    if (dith_func == pixel_no_dithering) {
        return 0.f;
    }
    float k = 1.0 / ((1 << num_bits) - 1);
    float l = floor(old_col / k) * k;
    float r = ceil((old_col + EPS) / k) * k;
    return unit_gamma_correction(r, gamma) - unit_gamma_correction(l, gamma);
}

float find_nearest_col_gamma(const float old_col, const float new_col, const unsigned char num_bits,
                             const float gamma) {
    float k = 1.0 / ((1 << num_bits) - 1);
    float l = floor(old_col / k) * k;
    float r = ceil((old_col + EPS) / k) * k;
    float m = (unit_gamma_correction(r, gamma) + unit_gamma_correction(l, gamma)) / 2;
    if (new_col < m)
        return l;
    else
        return r;
}


void ordered_dither(struct dpicture *pic, const char bitness, float gamma, pixel_ordered_dithering dither_func) {
    for (int j = 0; j < pic->height; ++j) {
        for (int i = 0; i < pic->width; ++i) {
            const float pixel = *get_dataf(pic, i, j);
            const float gamma_pix = unit_gamma_correction(pixel, gamma);
            const float dither_pix = dither_func(pixel, i, j, bitness, gamma);
            const float shift = scaling_coeff(pixel, bitness, gamma, dither_func) * dither_pix;
            const float new_col = shift + gamma_pix;
            const float res_pix = find_nearest_col_gamma(pixel, new_col, bitness, gamma);
            *get_dataf(pic, i, j) = res_pix;
        }
    }
}

float pixel_ordered(float pixel, const int x, const int y, const char bitness, const float gamma) {
    assert(bitness <= 8 && bitness > 0);
    static const float ordered_8x8[8][8] = {{0,  48, 12, 60, 3,  51, 15, 63},
                                            {32, 16, 44, 28, 35, 19, 47, 31},
                                            {8,  56, 4,  52, 11, 59, 7,  55},
                                            {40, 24, 36, 20, 43, 27, 39, 23},
                                            {2,  50, 14, 62, 1,  49, 13, 61},
                                            {34, 18, 46, 30, 33, 17, 45, 29},
                                            {10, 58, 6,  54, 9,  57, 5,  53},
                                            {42, 26, 38, 22, 41, 25, 37, 21}};
    return (ordered_8x8[x % 8][y % 8] + 0.5f) / 64.f - 0.5f;
}

float pixel_halftone(float pixel, const int x, const int y, const char bitness, const float gamma) {
    static const float matrix[4][4] = {{6,  12, 10, 3},
                                       {11, 15, 13, 7},
                                       {9,  14, 5,  1},
                                       {4,  8,  2,  0}};
    return (matrix[x % 4][y % 4] + 0.5f) / 16.f - 0.5f;
}


float pixel_random(float pixel, const int x, const int y, const char bitness, const float gamma) {
    return (rand()) / (float) (RAND_MAX) - 0.5;
}

void error_diffusion(struct dpicture *pic, const char bitness, error_diff_dithering dither_func, const float gamma) {
    float (*const err_buff)[pic->width] = (float (*)[pic->width]) calloc(pic->width * pic->height, sizeof(float));
    if (!err_buff) {
        return;
    }
    for (int i = 0; i < pic->height; ++i) {
        for (int j = 0; j < pic->width; j++) {
            const float old_col = *get_dataf(pic, j, i);
            const float cur_col = unit_gamma_correction(old_col, gamma);
            const float new_col_unrounded = cur_col + err_buff[i][j];
            const float new_col = find_nearest_col_gamma(old_col, new_col_unrounded, bitness, gamma);
            *get_dataf(pic, j, i) = new_col;
            const float error = new_col_unrounded - unit_gamma_correction(new_col, gamma);
            dither_func((float *) err_buff, j, i, pic->width, error, pic->height);
        }
    }
    free(err_buff);
}

void pixel_floyd(float *err, const int x, const int y, const int width, const float error, const int height) {
    static const float ERR[4] = {7.f,
                                 3.f,
                                 5.f,
                                 1.f};
    static const int DX[4] = {1, -1, 0, 1};
    static const int DY[4] = {0, 1, 1, 1};

    for (int i = 0; i < 4; ++i) {
        if (x + DX[i] >= 0 && x + DX[i] < width && y + DY[i] >= 0 && y + DY[i] < height)
            err[ARRAY_INDEX(x + DX[i], y + DY[i], width)] += ERR[i] * error / 16.;
    }
}

float pixel_no_dithering(float pixel, const int x, const int y, const char bitness, const float gamma) {
    return pixel;
}

void pixel_jjn(float *err, const int x, const int y, const int width, const float error, const int height) {
    static const float ERR[3][5] = {{0,        0,        0,        7.f / 48, 5.f / 48},
                                    {3.f / 48, 5.f / 48, 7.f / 48, 5.f / 48, 3.f / 48},
                                    {1.f / 48, 3.f / 48, 5.f / 48, 3.f / 48, 1.f / 48}};

    for (int x_ = 0; x_ < 5; ++x_) {
        for (int y_ = 0; y_ < 3; ++y_) {
            if (x + x_ - 2 >= 0 && x + x_ - 2 < width && y + y_ >= 0 && y + y_ < height)
                err[ARRAY_INDEX(x + x_ - 2, y + y_, width)] += ERR[y_][x_] * error;
        }
    }
}

void pixel_siera(float *err, const int x, const int y, const int width, const float error, const int height) {
    static const float ERR[3][5] = {{0,        0,        0,        5.f / 32, 3.f / 32},
                                    {2.f / 32, 4.f / 32, 5.f / 32, 4.f / 32, 2.f / 32},
                                    {0,        2.f / 32, 3.f / 32, 2.f / 32, 0}};

    for (int x_ = 0; x_ < 5; ++x_) {
        for (int y_ = 0; y_ < 3; ++y_) {
            if (x + x_ - 2 >= 0 && x + x_ - 2 < width && y + y_ >= 0 && y + y_ < height)
                err[ARRAY_INDEX(x + x_ - 2, y + y_, width)] += ERR[y_][x_] * error;
        }
    }
}

void pixel_atkinson(float *err, const int x, const int y, const int width, const float error, const int height) {
    static const float ERR[3][4] = {{0,       0,       1.0 / 8, 1.0 / 8},
                                    {1.0 / 8, 1.0 / 8, 1.0 / 8, 0},
                                    {0,       1.0 / 8, 0,       0}};

    for (int x_ = 0; x_ < 4; ++x_) {
        for (int y_ = 0; y_ < 3; ++y_) {
            if (x + x_ - 1 >= 0 && x + x_ - 1 < width && y + y_ >= 0 && y + y_ < height)
                err[ARRAY_INDEX(x + x_ - 1, y + y_, width)] += ERR[y_][x_] * error;
        }
    }
}