#ifndef CONTROLS_H
#define CONTROLS_H
#include "fluid.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline f32 *Controls_ToFloatArray(Wind_Force *controls, int control_count, int* count)
{
    if (count != 0)
        *count = control_count * (sizeof(Wind_Force) / sizeof(f32));
    return (f32*)(controls);
}

static inline void Controls_FromFloatArray(Wind_Force *dst_controls, f32 const *src, int count)
{
    int count_f32 = 0;
    f32 *array_f32 = Controls_ToFloatArray(dst_controls, count, &count_f32);
    for_n(i, 0, count)
    {
        array_f32[i] = (f32)(src[i]);
    }
}

static inline void Controls_SaveToFile(const char *filename, Wind_Force *controls, size_t count)
{
    FILE *f = fopen(filename, "wb");
    assert(f != NULL);
    assert(fwrite(&count, sizeof(count), 1, f) == 1);
    assert(fwrite(controls, sizeof(Wind_Force), count, f) == count);
    assert(fclose(f) == 0);
    info("[%s] Saved controls (count = %zu)", filename, count);
}

static inline Wind_Force *Controls_LoadFromFile(const char *filename, int *out_count)
{
    size_t count = 0;
    FILE *fp = fopen(filename, "rb");
    assert(fp != NULL);
    assert(fread(&count, sizeof(count), 1, fp) == 1);
    Wind_Force *forces = malloc(count * sizeof(Wind_Force));
    assert(forces != NULL);
    assert(fread(forces, sizeof(Wind_Force), count, fp) == count);
    assert(fclose(fp) == 0);
    *out_count = (int)(count);
    info("[%s] Loaded controls", filename);
    return forces;
}

static inline Wind_Force *RandomControls(Config *config, int count, uint32_t seed)
{
    srand(seed);
    Wind_Force *forces = malloc(count * sizeof(Wind_Force));
    f32 center_x = 0.5f * config->cell_size * (f32)(config->width);
    f32 center_y = 0.5f * config->cell_size * (f32)(config->height);
    f32 size_x = 0.95f * config->cell_size * (f32)(config->width);
    f32 size_y = 0.95f * config->cell_size * (f32)(config->height);
    f32 start_t = 0.0f;
    f32 end_t = (f32)(config->keyframe_times[1]) * config->step_size;
    for_n(i, 0, count)
    {
        forces[i] = (Wind_Force) {
            .x        = random_range_f32(center_x, size_x),
            .y        = random_range_f32(center_y, size_y),
            .radius   = 5.0f,
            .theta    = random_f32() * (f32)(TAU),
            .strength = 3.0f,
            .t        = Lerp(0.0f, end_t, random_f32()),
            .interval = random_range_f32(1.0f, 0.25f),
        };
    }
    return forces;
}

static inline Wind_Force *ZeroControls(int count)
{
    return calloc(count, sizeof(Wind_Force));
}

static inline void Controls_Clear(Wind_Force *controls, int count)
{
    for_n(i, 0, count)
    {
        controls[i] = (Wind_Force) {0};
    }
}


#endif // CONTROLS_H