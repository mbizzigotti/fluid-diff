#ifndef FLUID_H
#define FLUID_H
#include <stdint.h>

typedef float f32;

typedef struct {
    f32 *s;
    f32 *u; // X-velocity
    f32 *v; // Y-velocity
    f32 *m; // smoke density
    f32 *t1;
    f32 *t2;
    f32 cell_size;
    int cell_count;
    int grid_dim_x;
    int grid_dim_y;
} Fluid;

typedef struct {
    f32 x;        // position
    f32 y;
    f32 radius;   // how spread out the force is
    f32 theta;    // direction of the force
    f32 strength; // how strong the force is
    f32 t;        // when to apply the force
    f32 interval; // how long to apply the force
} Wind_Force;

typedef struct {
    f32 *values;
    int dim_x;
    int dim_y;
} Keyframe;

typedef struct {
    int num_timesteps;
    f32 step_size;
    f32 final_t;
} Parameters;

#define X_METHODS \
    X(gd,   Gradient_Decent) \
    X(adam, ADAM) \

enum Method {
#define X(short, name) Method_ ## name,
    X_METHODS
#undef X
};

typedef struct {
    const char *name;
    int framerate;
    int method;
    f32 step_size;
    f32 cell_size;
    int width;
    int height;
    int num_controls;
    int num_keyframes;
    int keyframe_times[16];
    const char *keyframes[16];
} Config;

typedef struct {
    Wind_Force *mean;
    Wind_Force *velocity;
} Adam;

#ifdef INCLUDE_OPTIMIZATION

typedef struct {
    char        output_dir[128];
    Fluid       fluid;
    Fluid       d_fluid;
    Wind_Force *controls;
    Wind_Force *d_controls;
    Keyframe    initial;
    Keyframe    target;
    Parameters  parameters;
    int         control_count;
    int         iter;
} Fluid_Objective_Data;

#endif // INCLUDE_OPTIMIZATION

#endif // FLUID_H