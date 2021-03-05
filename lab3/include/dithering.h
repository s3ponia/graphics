#ifndef DITHERING_H
#define DITHERING_H

struct dpicture;

#define NO_DITHERING 0
#define ORDERED_DITHERING 1
#define RANDOM_DITHERING 2
#define FLOYD_STEINBERG_DITHERING 3
#define JJN_DITHERING 4
#define SIERA_DITHERING 5
#define ATKINSON_DITHERING 6
#define HALFTONE 7

typedef float (*pixel_ordered_dithering)(float, int, int, int, const float gamma);

void ordered_dither(struct dpicture *pic, const char bitness, float gamma, pixel_ordered_dithering dither_func);

float pixel_ordered(float pixel, const int x, const int y, const char bitness, const float gamma);

float pixel_random(float pixel, const int x, const int y, const char bitness, const float gamma);

float pixel_halftone(float pixel, const int x, const int y, const char bitness, const float gamma);

float pixel_no_dithering(float pixel, const int x, const int y, const char bitness, const float gamma);

typedef void (*error_diff_dithering)(float *, int, int, int, float, int);

void error_diffusion(struct dpicture *pic, const char bitness, error_diff_dithering dither_func, const float gamma);

void pixel_floyd(float *err, const int x, const int y, const int width, const float error, const int height);

void pixel_jjn(float *err, const int x, const int y, const int width, const float error, const int height);

void pixel_siera(float *err, const int x, const int y, const int width, const float error, const int height);

void pixel_atkinson(float *err, const int x, const int y, const int width, const float error, const int height);

int fill_gradient(struct dpicture *p);

#endif
