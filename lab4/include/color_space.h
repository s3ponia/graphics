#ifndef COLOR_SPACE_H
#define COLOR_SPACE_H

struct picture;

typedef enum {
    RGB = 0, HSL, HSV, YCbCr_601, YCbCr_709, YCoCg, CMY
} color_space;

typedef struct {
    char data[256];
} filename;

typedef struct {
    struct picture *sources[3];
} color_sources;

typedef void (*to_rgb_pixel_func)(unsigned char *s1, unsigned char *s2, unsigned char *s3);

typedef void (*from_rgb_pixel_func)(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void hsl_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void hsv_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void YCbCr_601_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void YCbCr_709_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void YCoCg_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void CMY_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

static void noop(unsigned char *s1, unsigned char *s2, unsigned char *s3) {}

void hsl_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void hsv_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void YCbCr_601_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void YCbCr_709_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void YCoCg_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

void CMY_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3);

#endif
