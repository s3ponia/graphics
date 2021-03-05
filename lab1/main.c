#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define CHUNK 4096
#define READ_CHUNK CHUNK
#define SUCCESS 0
#define LOGIC_ERROR 1
#define FILE_ERROR 2
#define OVERFLOW_ERROR 3
#define NOMEM 4
#define PARSE_ERROR 5

enum type {
    P5, P6
};

struct picture {
    size_t width;
    size_t height;
    int max_color;
    int pixel_size;
    enum type type;
    char data[];
};

int swap_mem(char *p1, char *p2, size_t size) {
    char temp[CHUNK];
    while (size > 0) {
        if (size > CHUNK) {
            memcpy(temp, p1, CHUNK);
            memcpy(p1, p2, CHUNK);
            memcpy(p2, temp, CHUNK);
        } else {
            memcpy(temp, p1, size);
            memcpy(p1, p2, size);
            memcpy(p2, temp, size);
            return SUCCESS;
        }
        p1 += CHUNK;
        p2 += CHUNK;
        size -= CHUNK;
    }
    return SUCCESS;
}

int invert1(struct picture *picture) {
    size_t data_size = picture->width * picture->height * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    for (size_t i = 0; i < data_size; ++i) {
        if (picture->max_color < picture->data[i])
            return LOGIC_ERROR;
        picture->data[i] = picture->max_color - picture->data[i];
    }
    return SUCCESS;
}

int invert2(struct picture *picture) {
    size_t data_size = picture->width * picture->height * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    for (size_t i = 0; i < data_size; i += 2) {
        int value = (picture->data[i] << sizeof(char)) + picture->data[i + 1];
        if (picture->max_color < value)
            return LOGIC_ERROR;
        value = picture->max_color - value;
        picture->data[i] = value >> sizeof(char);
        picture->data[i + 1] = ~(~0u << sizeof(char)) & value;
    }
    return SUCCESS;
}

int invert(struct picture *picture) {
    if (picture->pixel_size == 2)
        return invert2(picture);
    else
        return invert1(picture);
}

int reverse(char *b, size_t size, enum type mode) {
    if (b == NULL) {
        return LOGIC_ERROR;
    }
    size_t increment = (mode == P5 ? 1 : 3);
    for (size_t i = 0; i < size / 2; i += increment) {
        swap_mem(b + i, b + (size - i - increment), increment);
    }
    return SUCCESS;
}

int horizontal_flip(struct picture *picture) {
    size_t width = picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    for (size_t i = 0; i < picture->height; ++i) {
        int ret;
        if ((ret = reverse(picture->data + i * width, width, picture->type)) != SUCCESS) {
            return ret;
        }
    }
    return SUCCESS;
}

int vertical_flip(struct picture *picture) {
    size_t width = picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    for (size_t i = 0; i < picture->height / 2; ++i) {
        swap_mem(picture->data + i * width, picture->data + (picture->height - i - 1) * width, width);
    }
    return SUCCESS;
}

int transpose(struct picture *picture) {
    size_t increment = (picture->type == P5 ? 1 : 3);
    size_t size = picture->width * picture->height * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    char *data = malloc(size * sizeof(char));
    if (data == NULL)
        return NOMEM;
    size_t m = picture->height;
    size_t n = picture->width;
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            memcpy(data + j * m * increment + i * increment, picture->data + i * n * increment + j * increment,
                   increment);
        }
    }

    memcpy(picture->data, data, size);

    free(data);

    picture->width = m;
    picture->height = n;

    return SUCCESS;
}

int rotate_right(struct picture *picture) {
    int ret;
    if ((ret = transpose(picture)) != SUCCESS) {
        return ret;
    }
    if ((ret = horizontal_flip(picture)) != SUCCESS) {
        return ret;
    }
    return SUCCESS;
}

int rotate_left(struct picture *picture) {
    int ret;
    if ((ret = transpose(picture)) != SUCCESS) {
        return ret;
    }
    if ((ret = vertical_flip(picture)) != SUCCESS) {
        return ret;
    }
    return SUCCESS;
}

int transform(struct picture *picture, long transform) {
    switch (transform) {
        case 0:
            return invert(picture);
        case 1:
            return horizontal_flip(picture);
        case 2:
            return vertical_flip(picture);
        case 3:
            return rotate_right(picture);
        case 4:
            return rotate_left(picture);
        default:
            return LOGIC_ERROR;
    }
}

int read_header(const char *data, size_t data_size, enum type *type, size_t *width, size_t *height, int *max_color,
                int *pixel_size, const char **end) {
    if (data_size < 2 || *data != 'P') {
        return PARSE_ERROR;
    }

    ++data;

    switch (*data) {
        case '5':
            *type = P5;
            break;
        case '6':
            *type = P6;
            break;
        default:
            return PARSE_ERROR;
    }

    ++data;

    data_size -= 2;

    char *check = NULL;

    errno = 0;
    *width = strtoul(data, &check, 10);

    if (errno) {
        return PARSE_ERROR;
    }

    data_size -= check - data - 1;
    data = check + 1;

    errno = 0;
    *height = strtoul(data, &check, 10);

    if (errno) {
        return PARSE_ERROR;
    }

    data_size -= check - data;
    data = check;

    errno = 0;
    *max_color = strtol(data, &check, 10);

    if (errno) {
        return PARSE_ERROR;
    }

    data_size -= check - data - 1;
    data = check + 1;

    *end = data;

    *pixel_size = 1;

    if (*max_color > 255ul) {
        *pixel_size = 2;
    }

    if (*type == P5 && (*pixel_size * *width * *height > data_size)) {
        return LOGIC_ERROR;
    }

    if (*type == P6 && (3 * *pixel_size * *width * *height > data_size)) {
        return LOGIC_ERROR;
    }

    return SUCCESS;
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

int save_picture(struct picture *picture, FILE *out) {
    if (out == NULL) {
        return LOGIC_ERROR;
    }
    if (fprintf(out, "%s\n%zu %zu\n%d\n", picture->type == P5 ? "P5" : "P6", picture->width, picture->height,
                picture->max_color) < 0) {
        return FILE_ERROR;
    }
    size_t data_size = picture->height * picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    if (fwrite(picture->data, sizeof(picture->data[0]), data_size, out) < data_size) {
        return FILE_ERROR;
    }
    return SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage:\n%s <inputFileName> <outputFileName> <transformation>", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "rb");
    if (input_file == NULL) {
        perror("can't open input file.");
        return EXIT_FAILURE;
    }

    FILE *output_file = fopen(argv[2], "wb");
    if (output_file == NULL) {
        fclose(input_file);
        perror("can't open output file.");
        return EXIT_FAILURE;
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
        fclose(input_file);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    char *test = NULL;
    long transf = strtol(argv[3], &test, 10);
    if (*test != '\0') {
        fprintf(stderr, "%s: not int.", argv[3]);
        fclose(input_file);
        fclose(output_file);
        free(data);
        return EXIT_FAILURE;
    }

    struct picture *picture = malloc(sizeof(struct picture) + size * sizeof(char));

    if(picture == NULL) {
        fprintf(stderr, "NOMEM: can't allocate memory for picture.");
        fclose(input_file);
        fclose(output_file);
        free(data);
        return EXIT_FAILURE;
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
        fclose(input_file);
        fclose(output_file);
        free(data);
        free(picture);
        return EXIT_FAILURE;
    }

    memcpy(picture->data, end, sizeof(data[0]) * picture->height * picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3));
    if (transform(picture, transf) != SUCCESS) {
        fprintf(stderr, "transform error.");
        fclose(input_file);
        fclose(output_file);
        free(data);
        free(picture);
        return EXIT_FAILURE;
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
        fclose(input_file);
        fclose(output_file);
        free(data);
        free(picture);
        return EXIT_FAILURE;
    }

    fclose(input_file);
    fclose(output_file);
    free(data);
    free(picture);
    return EXIT_SUCCESS;
}
