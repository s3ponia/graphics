#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include "picture.h"

int invert1(struct picture *picture);

int invert2(struct picture *picture);

int invert(struct picture *picture);

int horizontal_flip(struct picture *picture);

int reverse(char *b, size_t size, enum type mode);

int vertical_flip(struct picture *picture);

int transpose(struct picture *picture);

int rotate_right(struct picture *picture);

int rotate_left(struct picture *picture);

int transform(struct picture *picture, long transform);

#endif
