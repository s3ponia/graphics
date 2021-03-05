#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "../include/utility.h"
#include "../include/defines.h"
#include "../include/picture.h"

int swap_mem(void *p1, void *p2, size_t size) {
    char temp[CHUNK];
    while (size > 0) {
        size_t copy_size = size;
        if (size > CHUNK) {
            copy_size = CHUNK;
        }
        memcpy(temp, p1, copy_size);
        memcpy(p1, p2, copy_size);
        memcpy(p2, temp, copy_size);

        p1 += copy_size;
        p2 += copy_size;
        size -= copy_size;
    }
    return SUCCESS;
}

double gamma_correction(double value, double gamma) {
    if (gamma == 0)
    {
        if (value / 255. <= 0.04045) return value / 12.92;
        else return 255. * pow((value / 255. + 0.055) / 1.055, 2.4);
    }

    return fminf(255.f, fmaxf(0.f, 255. * pow(value / 255., gamma)));
}

double gamma_back_correction(double data, double gamma) {
    return gamma_correction(data, 1. / gamma);
}

const unsigned char *const_get_data(const picture *pic, int x, int y) {
    if (pic == NULL || pic->type == P5 && (pic->width <= x || pic->height <= y) ||
        pic->type == P6 && (pic->width * 3 <= x || pic->height * 3 <= y) || x < 0 || y < 0)
    assert(!(pic == NULL || pic->type == P5 && (pic->width <= x || pic->height <= y) ||
             pic->type == P6 && (pic->width * 3 <= x || pic->height * 3 <= y) || x < 0 || y < 0));
    if (pic->type == P5)
        return pic->data + x + y * pic->width;
    else if (pic->type == P6)
        return pic->data + x * 3 + y * pic->width * 3;

    assert(false);
}


unsigned char *get_data(picture *pic, int x, int y) {
    if (pic == NULL || pic->type == P5 && (pic->width <= x || pic->height <= y) ||
        pic->type == P6 && (pic->width * 3 <= x || pic->height * 3 <= y) || x < 0 || y < 0)
    assert(!(pic == NULL || pic->type == P5 && (pic->width <= x || pic->height <= y) ||
             pic->type == P6 && (pic->width * 3 <= x || pic->height * 3 <= y) || x < 0 || y < 0));
    if (pic->type == P5)
        return pic->data + x + y * pic->width;
    else if (pic->type == P6)
        return pic->data + x * 3 + y * pic->width * 3;

    assert(false);
}

int read_all(FILE *in, char **output_data, size_t *size) {
    char *data = NULL;
    char *temp = NULL;
    size_t used = 0;
    size_t capacity = 0;
    size_t n;

    if (in == NULL || output_data == NULL || size == NULL) {
        return LOGIC_ERROR;
    }

    do {
        if (capacity < used + READ_CHUNK + 1) {
            // overflow check
            if (capacity < used) {
                free(data);
                return OVERFLOW_ERROR;
            }

            capacity *= 1.5;
            if (capacity < used + READ_CHUNK + 1) {
                capacity = used + READ_CHUNK + 1;
            }

            temp = realloc(data, capacity);
            if (temp == NULL) {
                free(data);
                return NOMEM;
            }
            data = temp;
        }
        n = fread(data + used, sizeof(char), READ_CHUNK, in);
        used += n;
    } while (!ferror(in) && !feof(in) && (n != 0));

    if (ferror(in)) {
        free(data);
        return FILE_ERROR;
    }

    data = temp;

    *output_data = data;
    *size = used;

    return SUCCESS;
}

float *get_dataf(struct dpicture *pic, int x, int y) {
    if (pic == NULL || pic->width <= x || pic->height <= y || x < 0 || y < 0)
        assert(!(pic == NULL || pic->width <= x || pic->height <= y || x < 0 || y < 0));
    return pic->data + x + y * pic->width;
}
