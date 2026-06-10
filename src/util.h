#ifndef UTIL_H
#define UTIL_H
#include "fluid.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TAU 6.283185307179586

#define ERROR   "\x1b[91mERROR\x1b[0m"
#define WARNING "\x1b[93mWARNING\x1b[0m"
#define array_count(A) (sizeof(A)/sizeof(A[0]))
#define for_n(I,A,B) for (int I = (A); I < (B); I++)
#define info(FMT,...) printf("\x1b[92mINFO: " FMT "\x1b[0m\n", __VA_ARGS__)

int min_int(int a, int b)
{
    return a < b ? a : b;
}

int max_int(int a, int b)
{
    return a > b ? a : b;
}

f32 clamp_f32(float x, float min, float max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

f32 random_f32(void)
{
    return (f32)(rand()) / (f32)(RAND_MAX);
}

f32 random_range_f32(f32 avg, f32 spread)
{
    return spread * (random_f32() - 0.5f) + avg;
}

f32 luminance(f32 r, f32 g, f32 b)
{
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

int IsValidKeyframe(Fluid *fluid, Keyframe *frame)
{
    return (fluid->grid_dim_x == frame->dim_x + 2) && (fluid->grid_dim_y == frame->dim_y + 2);
}

void *alloc(size_t size, size_t n)
{
    size_t const alignment = 256;
    size_t a = size * n;
    size_t b = (a + alignment - 1) / alignment;
    size_t c = b * alignment;
    return aligned_alloc(alignment, c);
}

static inline const char* clone_cstring(const char *cstr)
{
    size_t length = strlen(cstr);
    char *new_cstr = malloc(length + 1);
    memcpy(new_cstr, cstr, length);
    new_cstr[length] = 0; // null terminate
    return new_cstr;
}

static inline void parse_f32(const char *str, float *output)
{
    char *end = NULL;
    float value = strtof(str, &end);
    if (value == 0.0f && end == str)
    {
        printf(WARNING": Could not convert %s to a f32 value.\n", str);
        return;
    }
    *output = value;
}

static inline void parse_int(const char *str, int *output)
{
    char *end = NULL;
    long long value = strtoll(str, &end, 10);
    if (value == 0 && end == str)
    {
        printf(WARNING": Could not convert %s to a int value.\n", str);
        return;
    }
    *output = (int)(value);
}

static inline void parse_method(const char *str, int *output)
{
    if (0);
#define X(short, name) else if(0==strcmp(str, #short)) *output = Method_ ## name;
    X_METHODS
#undef X
    else printf(WARNING": Unknown optimization method: %s.\n", str);
}

static inline const char *Method_Name(int method)
{
    switch (method)
    {
#define X(short, name) case Method_ ## name: return #short;
    X_METHODS
#undef X
    default: return "unknown";
    }
}

void CreateDirWithTimestamp(const char *prefix, const char *name, char *folder)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    snprintf(folder, 128, "%s_%s_%s", prefix, name, timestamp);

    if (mkdir(folder, 0755) == 0) {
        info("Created Output Folder: %s", folder);
    } else {
        perror("mkdir");
        exit(1);
    }
}

void CreateDir(const char *prefix, const char *name, char *folder)
{
    snprintf(folder, 64, "%s_%s", prefix, name);
    if (mkdir(folder, 0755) == 0) {
        info("Created Output Folder: %s", folder);
    } else {
        perror("mkdir");
        exit(1);
    }
}

#define CreateOutputDir(N, O) CreateDirWithTimestamp("out", N, O)
#define CreateRenderDir(N, O) CreateDir("anim", N, O)

#endif // UTIL_H