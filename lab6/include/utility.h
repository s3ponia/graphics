#ifndef UTILITY_H
#define UTILITY_H

#include <ctype.h>
#include <stdio.h>

typedef struct picture picture;
struct dpicture;

#define EPS 1e-25

#define READ_FLOAT(output, data, failure, function) do {\
    errno = 0;\
    char *check;\
    (output) = function((data), &check);\
    if (errno) {\
        failure\
        return EXIT_FAILURE;\
    }\
    if (*check != '\0') {\
        failure\
        return EXIT_FAILURE;\
    }\
} while(0)

#define READ_INT(output, data, failure, function) do {\
    errno = 0;\
    char *check;\
    (output) = function((data), &check, 10);\
    if (errno) {                                                 \
        failure                                                 \
    }\
    if (*check != '\0') {                                        \
        failure                                                  \
    }\
} while(0)

int swap_mem(void *p1, void *p2, size_t size);

double gamma_correction(double value, double gamma);

double gamma_back_correction(double data, double gamma);

unsigned char *get_data(picture *pic, int x, int y);

const unsigned char *const_get_data(const picture *pic, int x, int y);

float *get_dataf(struct dpicture *pic, int x, int y);

int read_all(FILE *in, char **output_data, size_t *size);

#endif
