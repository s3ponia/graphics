#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "../include/color_space.h"
#include "../include/picture.h"
#include "../include/utility.h"

static float hue_to_rgb(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1. / 6.) return p + (q - p) * 6 * t;
    if (t < 1. / 2.) return q;
    if (t < 2. / 3.) return p + (q - p) * (2. / 3. - t) * 6;
    return p;
}

void hsl_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float h = *s1 / 255.f;
    const float s = *s2 / 255.f;
    const float l = *s3 / 255.f;

    float r;
    float g;
    float b;

    if (s == 0) {
        r = 1;
        g = 1;
        b = 1;
    } else {
        const float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
        const double p = 2 * l - q;
        r = hue_to_rgb(p, q, h + 1. / 3.);
        g = hue_to_rgb(p, q, h);
        b = hue_to_rgb(p, q, h - 1. / 3.);
    }

    *s1 = round(255.f * r);
    *s2 = round(255.f * r);
    *s3 = round(255.f * r);
}

void hsv_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const int h = (int) round(*s1 / 255. * 360.);
    const float s = *s2 / 255.f;
    const float v = *s3 / 255.f;

    const float v_min = (100. - s) * v / 100.;
    const float a = (v - v_min) * (h % 60) / 60.f;
    const float v_inc = v_min + a;
    const float v_dec = v - a;
    const int h_i = (h / 60) % 6;

    const float rgb[6][3] = {
            [0] = {v, v_inc, v_min},
            [1] = {v_dec, v, v_min},
            [2] = {v_min, v, v_inc},
            [3] = {v_min, v_dec, v},
            [4] = {v_inc, v_min, v},
            [5] = {v, v_min, v_dec}
    };

    *s1 = rgb[h_i][0] * 255. / 100.;
    *s2 = rgb[h_i][1] * 255. / 100.;
    *s3 = rgb[h_i][2] * 255. / 100.;
}

void YCbCr_601_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float y = *s1;
    const float cb = *s2;
    const float cr = *s3;

    const float r = y + 1.5748f * (cr - 128.f);
    const float g = y - 0.18732427f * (cb - 128.f) - 0.46812427f * (cr - 128.f);
    const float b = y + 1.8556f * (cb - 128.f);

    *s1 = roundf(fminf(fmaxf(r, 0.f), 255.f));
    *s2 = roundf(fminf(fmaxf(g, 0.f), 255.f));
    *s3 = roundf(fminf(fmaxf(b, 0.f), 255.f));
}

void YCbCr_709_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float y = *s1;
    const float cb = *s2;
    const float cr = *s3;

    const float r = y + 1.402f * (cr - 128.f);
    const float g = y - 0.344136f * (cb - 128.f) - 0.714136 * (cr - 128.f);
    const float b = y + 1.772 * (cb - 128.f);

    *s1 = roundf(fminf(fmaxf(r, 0.f), 255.f));
    *s2 = roundf(fminf(fmaxf(g, 0.f), 255.f));
    *s3 = roundf(fminf(fmaxf(b, 0.f), 255.f));
}

static void to_rgb(color_sources sources, to_rgb_pixel_func func) {
    assert(sources.sources[0]->type == P5);
    assert(sources.sources[1]->type == P5);
    assert(sources.sources[2]->type == P5);
    assert(sources.sources[0]->width == sources.sources[1]->width &&
           sources.sources[1]->width == sources.sources[2]->width &&
           sources.sources[0]->width == sources.sources[2]->width &&
           sources.sources[0]->height == sources.sources[1]->height &&
           sources.sources[1]->height == sources.sources[2]->height &&
           sources.sources[0]->height == sources.sources[2]->height
    );

    for (size_t i = 0; i < sources.sources[0]->width * sources.sources[0]->height; ++i) {
        func(&sources.sources[0]->data[i],
             &sources.sources[1]->data[i],
             &sources.sources[2]->data[i]);
    }
}


void hsl_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float r = *s1 / 255.f;
    const float g = *s2 / 255.f;
    const float b = *s3 / 255.f;

    const float max_val = fmaxf(r, fmaxf(g, b));
    const float min_val = fminf(r, fminf(g, b));

    float h;
    float s;
    const float l = (max_val + min_val) / 2;

    if (max_val == min_val) {
        h = 0;
        s = 0;
    } else {
        const float d = max_val - min_val;
        s = l > 0.5 ? d / (2 - max_val - min_val) : d / (max_val + min_val);
        if (fabsf(max_val - r) < EPS)
            h = (g - b) / d + (g < b ? 6 : 0);
        else if (fabsf(max_val - g) < EPS)
            h = (b - g) / d + 2;
        else if (fabsf(max_val - b) < EPS)
            h = (r - g) / d + 4;
    }


    *s1 = h;
    *s2 = s;
    *s3 = l;
}

void hsv_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float r = *s1 / 255.;
    const float g = *s2 / 255.;
    const float b = *s3 / 255.;

    const float max_val = fmaxf(r, fmaxf(g, b));
    const float min_val = fminf(r, fminf(g, b));

    int h;

    if (max_val == r && g >= b) {
        h = 60 * (g - b) / (max_val - min_val);
    } else if (max_val == r && g < b) {
        h = 60 * (g - b) / (max_val - min_val) + 360;
    } else if (max_val == g) {
        h = 60 * (b - r) / (max_val - min_val) + 120;
    } else if (max_val == b) {
        h = (r - g) / (max_val - min_val) + 240.;
    }

    float s;

    if (fabsf(max_val - 0.f) < EPS) {
        s = 0;
    } else {
        s = 1.f - (float) min_val / max_val;
    }

    const float v = max_val;

    *s1 = round(h * 255. / 360.);
    *s2 = round(s * 255.);
    *s3 = round(v * 255.);
}

void YCbCr_601_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    double r = *s1;
    double g = *s2;
    double b = *s3;

    double y = 0.299 * r + 0.587 * g + 0.114 * b;
    double cb = -0.168736 * r - 0.331264 * g + 0.5 * b + 128.0;
    double cr = 0.5 * r - 0.418688 * g - 0.081312 * b + 128.0;

    *s1 = round(fminf(fmaxf(y, 0.f), 255.f));
    *s2 = round(fminf(fmaxf(cb, 0.f), 255.f));
    *s3 = round(fminf(fmaxf(cr, 0.f), 255.f));
}

void YCbCr_709_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float r = *s1;
    const float g = *s2;
    const float b = *s3;

    const float y = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    const float cb = -0.11457211f * r - 0.38542789f * g + 0.5f * b + 128.f;
    const float cr = 0.5f * r - 0.45415291f * g - 0.04584709f * b + 128.f;

    *s1 = round(fminf(fmaxf(y, 0.f), 255.f));
    *s2 = round(fminf(fmaxf(cb, 0.f), 255.f));
    *s3 = round(fminf(fmaxf(cr, 0.f), 255.f));
}

void YCoCg_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float y = *s1;
    const float co = *s2 - 128.f;
    const float cg = *s3 - 128.f;

    const float r = y + co - cg;
    const float g = y + cg;
    const float b = y - co - cg;

    *s1 = round(fminf(fmaxf(r, 0.f), 255.f));
    *s2 = round(fminf(fmaxf(g, 0.f), 255.f));
    *s3 = round(fminf(fmaxf(b, 0.f), 255.f));
}

void YCoCg_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float r = *s1;
    const float g = *s2;
    const float b = *s3;

    const float y = 0.25f * r + 0.5f * g + 0.25f * b;
    const float co = 0.5f * r - 0.5f * b + 128.f;
    const float cg = -0.25f * r + 0.5f * g - 0.25f * b + 128.f;

    *s1 = round(fminf(fmaxf(y, 0.f), 255.f));
    *s2 = round(fminf(fmaxf(co, 0.f), 255.f));
    *s3 = round(fminf(fmaxf(cg, 0.f), 255.f));
}

void CMY_to_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float c = *s1 / 255.f;
    const float m = *s2 / 255.f;
    const float y = *s3 / 255.f;

    const float r = 255.f * (1.f - c);
    const float g = 255.f * (1.f - m);
    const float b = 255.f * (1.f - y);

    *s1 = round(fminf(fmaxf(r, 0.f), 255.f));
    *s2 = round(fminf(fmaxf(g, 0.f), 255.f));
    *s3 = round(fminf(fmaxf(b, 0.f), 255.f));
}

void CMY_from_rgb_pixel(unsigned char *s1, unsigned char *s2, unsigned char *s3) {
    const float r = *s1 / 255.f;
    const float g = *s2 / 255.f;
    const float b = *s3 / 255.f;

    const float c = 255.f * (1.f - r);
    const float m = 255.f * (1.f - g);
    const float y = 255.f * (1.f - b);

    *s1 = round(fminf(fmaxf(c, 0.f), 255.f));
    *s2 = round(fminf(fmaxf(m, 0.f), 255.f));
    *s3 = round(fminf(fmaxf(y, 0.f), 255.f));
}

void from_rgb(color_sources sources, from_rgb_pixel_func func) {
    assert(sources.sources[0]->type == P5);
    assert(sources.sources[1]->type == P5);
    assert(sources.sources[2]->type == P5);
    assert(sources.sources[0]->width == sources.sources[1]->width &&
           sources.sources[1]->width == sources.sources[2]->width &&
           sources.sources[0]->width == sources.sources[2]->width &&
           sources.sources[0]->height == sources.sources[1]->height &&
           sources.sources[1]->height == sources.sources[2]->height &&
           sources.sources[0]->height == sources.sources[2]->height
    );

    for (size_t i = 0; i < sources.sources[0]->width * sources.sources[0]->height; ++i) {
        func(&sources.sources[0]->data[i],
             &sources.sources[1]->data[i],
             &sources.sources[2]->data[i]);
    }
}