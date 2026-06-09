#include "fluid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR   "\x1b[91mERROR\x1b[0m"
#define WARNING "\x1b[93mWARNING\x1b[0m"
#define array_count(A) (sizeof(A)/sizeof(A[0]))

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
