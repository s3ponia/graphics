#ifndef PICTURE_H
#define PICTURE_H

#include <ctype.h>
#include <stdio.h>

enum type {
    P5, P6
};

typedef struct picture {
    size_t width;
    size_t height;
    int max_color;
    int pixel_size;
    enum type type;
    unsigned char data[];
} picture;

typedef struct dpicture {
    size_t width;
    size_t height;
    int max_color;
    enum type type;
    float data[];
} dpicture;

typedef struct point {
    int x;
    int y;
} point;

int read_header(const char *data, size_t data_size, enum type *type, size_t *width, size_t *height, int *max_color,
                int *pixel_size, const char **end);

int save_picture(struct picture *picture, FILE *out);

void line_from_to(picture *pic, point pf, point pt, unsigned char brightness, double gamma, double wd);

int picture_to_dpicture(picture *src, dpicture *out);

int dpicture_to_picture(dpicture *src, picture *out);

size_t picture_size(picture *pic);


#endif
