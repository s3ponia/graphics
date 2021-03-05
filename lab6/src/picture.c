#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "../include/picture.h"
#include "../include/defines.h"
#include "../include/utility.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) > (b) ? (b) : (a))
#define for_range(NAME, RANGE, CHECK_RANGE, BODY) \
do {                                 \
    const int_range RANGE___##NAME = (RANGE);   \
    const int_range CHECK_RANGE_##NAME = (CHECK_RANGE);   \
    const int START___##NAME  = max(min(RANGE___##NAME.start, RANGE___##NAME.end), CHECK_RANGE_##NAME.start); \
    const int END___##NAME = min(max(RANGE___##NAME.start, RANGE___##NAME.end), CHECK_RANGE_##NAME.end);        \
    for (int NAME = START___##NAME; NAME <= END___##NAME; ++NAME) {\
        BODY                                 \
    }\
} while(0)

typedef struct {
    double x;
    double y;
} vector;

typedef struct {
    double start;
    double end;
} line;

typedef struct {
    vector direction;
    vector start;
} function;

typedef struct {
    int start;
    int end;
} int_range;

typedef vector rectangle[4];

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
    long temp = strtol(data, &check, 10);

    if (errno) {
        return PARSE_ERROR;
    }

    if (temp > INT_MAX || temp < 0) {
        return PARSE_ERROR;
    }
    *max_color = temp;

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

int save_picture(struct picture *picture, FILE *out) {
    if (out == NULL) {
        return LOGIC_ERROR;
    }
    if (fprintf(out, "%s\n%lu %lu\n%d\n", picture->type == P5 ? "P5" : "P6", picture->width, picture->height,
                picture->max_color) < 0) {
        return FILE_ERROR;
    }
    size_t data_size = picture->height * picture->width * picture->pixel_size * (picture->type == P5 ? 1 : 3);
    if (fwrite(picture->data, sizeof(picture->data[0]), data_size, out) < data_size) {
        return FILE_ERROR;
    }
    return SUCCESS;
}

static double intersect_area_line(line l1, line l2) {
    if (l1.start > l2.start && l1.start < l2.end || l1.end > l2.start && l1.end < l2.end) {
        if (l1.start > l2.start) {
            return l1.end - l1.start;
        }
        return l1.end - l2.start;
    }
    return 0;
}


static double projection(vector point, vector vector) {
    return point.x * vector.x + point.y * vector.y;
}

static vector point_to_vec(point p) {
    return (vector) {.x = p.x, .y = p.y};
}

static double calc_y(double x, function func) {
    return func.direction.y * (x - func.start.x) / func.direction.x + func.start.y;
}

static double calc_x(double y, function func) {
    return func.direction.x * (y - func.start.y) / func.direction.y + func.start.x;
}

static double size(vector v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

static vector perpendicular(vector v) {
    return (vector) {-v.y, v.x};
}

static vector unit_vector(vector v) {
    return (vector) {v.x / size(v), v.y / size(v)};
}

static vector add(vector a, vector b) {
    return (vector) {.x = a.x + b.x, .y = a.y + b.y};
}

static vector substract(vector a, vector b) {
    return (vector) {.x = a.x - b.x, .y = a.y - b.y};
}

static vector multiply(vector v, double m) {
    return (vector) {.x = v.x * m, .y = v.y * m};
}

static int sign(int x) {
    return (x > 0) - (x < 0);
}

static bool less(vector a, vector b, vector center) {
    if (a.x - center.x >= 0 && b.x - center.x < 0)
        return true;
    if (a.x - center.x < 0 && b.x - center.x >= 0)
        return false;
    if (a.x - center.x == 0 && b.x - center.x == 0) {
        if (a.y - center.y >= 0 || b.y - center.y >= 0)
            return a.y > b.y;
        return b.y > a.y;
    }

    // compute the cross product of vectors (center -> a) x (center -> b)
    double det = (a.x - center.x) * (b.y - center.y) - (b.x - center.x) * (a.y - center.y);
    if (det < 0)
        return true;
    if (det > 0)
        return false;

    // points a and b are on the same line from the center
    // check which point is closer to the center
    double d1 = (a.x - center.x) * (a.x - center.x) + (a.y - center.y) * (a.y - center.y);
    double d2 = (b.x - center.x) * (b.x - center.x) + (b.y - center.y) * (b.y - center.y);
    return d1 > d2;
}

static void sort_to_clockwise(rectangle *rec) {
    vector center = {};
    for (int i = 0; i < sizeof(*rec) / sizeof((*rec)[0]); ++i) {
        center = add(center, (*rec)[i]);
    }
    center = multiply(center, 0.25);
    for (int i = 0; i < sizeof(*rec) / sizeof((*rec)[0]); ++i) {
        for (int j = i + 1; j < sizeof(*rec) / sizeof((*rec)[0]); ++j) {
            if (less((*rec)[j], (*rec)[i], center)) {
                vector temp = (*rec)[j];
                (*rec)[j] = (*rec)[i];
                (*rec)[i] = temp;
            }
        }
    }
}

static double triangle_area(const vector a, const vector b, const vector c) {
    const double area = ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) / 2.0;
    return (area > 0.0) ? area : -area;
}

static bool point_in_rect(vector point, const rectangle *rec) {
    const double area1 = triangle_area(point, (*rec)[0], (*rec)[1]);
    const double area2 = triangle_area(point, (*rec)[1], (*rec)[2]);
    const double area3 = triangle_area(point, (*rec)[2], (*rec)[3]);
    const double area4 = triangle_area(point, (*rec)[0], (*rec)[3]);

    return fabs(area1 + area2 + area3 + area4 -
                size(substract((*rec)[0], (*rec)[1])) * size(substract((*rec)[1], (*rec)[2]))) < 1e-6;
}

static double pixel_intersect_rect(vector pixel, rectangle *rec) {
    const int sz = 4;
    const double step = 1. / sz;
    int count_points = 0;
    for (int i = 0; i < sz; ++i) {
        for (int j = 0; j < sz; ++j) {
            vector point = add(pixel, (vector) {.x = step * i, .y = step * j});
            if (point_in_rect(point, rec)) {
                ++count_points;
            }
        }
    }
    return count_points / (double) (sz * sz);
}

static unsigned char linegamma_correction(unsigned char prev, unsigned char new, double proportion, double gamma) {
    const double a = pow(prev / 255., gamma);
    const double b = pow(new / 255., gamma);
    const double c = pow((1. - proportion) * a + proportion * b, 1. / gamma);
    return c > 255 ? 255 : round(c * 255);
}

void line_from_to(picture *pic, const point pf, const point pt, const unsigned char brightness, const double gamma,
                  const double wd) {
    assert(!(pic == NULL || pf.x < 0 || pf.x >= pic->width || pf.y < 0 || pf.y >= pic->height
             || pt.x < 0 || pt.x >= pic->width || pt.y < 0 || pt.y >= pic->height || wd <= 0 || gamma <= 0));

    const vector line_vec = (vector) {pt.x - pf.x, pt.y - pf.y};
    const vector line_dir = (pt.x > pf.x ? line_vec :
                             (pt.x == pf.x ? (pt.y > pf.y ? line_vec : multiply(line_vec, -1)) :
                              multiply(line_vec, -1)
                             )
    );

    const function line = (function) {.direction = unit_vector(
            line_dir), .start = point_to_vec(
            pf)};
    const double line_size = size((vector) {pt.x - pf.x, pt.y - pf.y});

    const vector top_line_point = point_to_vec(pf.x < pt.x ? pf : (pf.x == pt.x ? (pf.y > pt.y ? pt : pf) : pt));
    const vector bottom_line_point = point_to_vec(pt.x > pf.x ? pt : (pt.x == pf.x ? (pt.y < pf.y ? pf : pt) : pf));

    const vector perpendicular_line = perpendicular(line.direction);
    const vector width_direction = perpendicular_line.x > 0 ? perpendicular_line : multiply(perpendicular_line, -1);

    const function top_width = (function) {.direction = width_direction,
            .start = substract(top_line_point, multiply(perpendicular(line.direction), -wd / 2))};
    const function bottom_width = (function) {.direction = width_direction, .start = substract(
            bottom_line_point, multiply(perpendicular(line.direction), -wd / 2))};

    const function top_line = (function) {.direction = line.direction,
            .start = add(top_width.start, multiply(top_width.direction, wd))};
    const function bottom_line = (function) {.direction = line.direction, .start = top_width.start};

    rectangle rect =
            {
                    [0] = bottom_line.start,
                    [1] = top_line.start,
                    [2] = add(bottom_line.start, multiply(unit_vector(bottom_line.direction), line_size)),
                    [3] = add(top_line.start, multiply(unit_vector(bottom_line.direction), line_size))
            };
    sort_to_clockwise(&rect);

    const int_range x_range = {.start = 0, .end = pic->width - 1};
    const int_range y_range = {.start = 0, .end = pic->height - 1};

    const int_range x_rect_range = {.start = min(pt.x, pf.x) - wd,
            .end = max(pt.x, pf.x) + wd};
    const int_range y_rect_range = {.start = min(pt.y, pf.y) - wd,
            .end = max(pt.y, pf.y) + wd};

    for_range(y, y_rect_range, y_range,
              for_range(x, x_rect_range, x_range, {
                      *get_data(pic, x, y) = linegamma_correction(*get_data(pic, x, y), brightness,
                                                                  pixel_intersect_rect((vector) {.x = x, .y = y},
                                                                                       &rect),
                                                                  gamma);
              });
    );
}

int picture_to_dpicture(picture *src, dpicture *out) {
    if (out == NULL || src == NULL || src->type != P5) {
        errno = EINVAL;
        return EINVAL;
    }

    out->height = src->height;
    out->width = src->width;
    out->type = src->type;
    out->max_color = src->max_color;

    for (size_t i = 0; i < src->width * src->height; ++i) {
        out->data[i] = src->data[i] / 255.;
    }

    return 0;
}

int dpicture_to_picture(dpicture *src, picture *out) {
    if (out == NULL || src == NULL || src->type != P5) {
        errno = EINVAL;
        return EINVAL;
    }

    out->height = src->height;
    out->width = src->width;
    out->type = src->type;
    out->max_color = src->max_color;

    for (size_t i = 0; i < src->width * src->height; ++i) {
        out->data[i] = round(255. * src->data[i]);
    }

    return 0;
}

size_t picture_size(picture *pic) {
    assert(pic != NULL);
    return (pic->type == P5 ? 1 : 3) * pic->width * pic->height;
}
