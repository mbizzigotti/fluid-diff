#ifdef NDEBUG
#undef NDEBUG // Hour of debugging... thanks CMake!!!!!
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include "fluid.h"
#include "nlopt.h"

#define PROJECTION_ITERATIONS  100
#define HEAT_CONSTANT          6.0f
#define INTERACTION_RADIUS     20
#define CONTROL_PENALTY        0.00001f

#define for_n(I,A,B) for (int I = (A); I < (B); I++)
#define get(FIELD,X,Y) (fluid->FIELD[(Y)*(fluid->grid_dim_x)+(X)])
#define copy(DST,SRC) for (int _i = 0; _i < fluid->cell_count; _i++) fluid->DST[_i] = fluid->SRC[_i]
#define info(FMT,...) printf("\x1b[92mINFO: " FMT "\x1b[0m\n", __VA_ARGS__)
#if 0
#   define LOG(...) printf(__VA_ARGS__)
#else
#   define LOG(...) (void)0
#endif
float global_regularization_term;
float global_data_term;
#if 0
#   define ACCUMULATE_REGULARIZATION(X) global_regularization_term += (X)
#   define ACCUMULATE_DATA(X)           global_data_term += (X)
#else
#   define ACCUMULATE_REGULARIZATION(X)
#   define ACCUMULATE_DATA(X)
#endif

typedef enum {
    Interact_Smoke,
    Interact_Velocity,
    Interact_Smoke_And_Velocity,
    Interact_Obstacle,
} Interection_Type;

int enzyme_out, enzyme_dup, enzyme_dupnoneed, enzyme_const;
float __enzyme_fwddiff(void*, ...);
float __enzyme_autodiff(void*, ...);
void __enzyme_autodiff_void(void*, ...);

int min_int(int a, int b)
{
    return a < b ? a : b;
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

int IsValidKeyframe(Fluid *fluid, Keyframe *frame)
{
    return (fluid->grid_dim_x == frame->dim_x + 2) && (fluid->grid_dim_y == frame->dim_y + 2);
}

f32 *Controls_ToFloatArray(Wind_Force *controls, int control_count, int* count)
{
    if (count != 0)
        *count = control_count * (sizeof(Wind_Force) / sizeof(f32));
    return (f32*)(controls);
}

void *alloc(size_t size, size_t n)
{
    size_t const alignment = 256;
    size_t a = size * n;
    size_t b = (a + alignment - 1) / alignment;
    size_t c = b * alignment;
    return aligned_alloc(alignment, c);
}

Fluid Fluid_Create(f32 cell_size, int width, int height)
{
    int dim_x = width + 2;
    int dim_y = height + 2;
    int cell_count = dim_x * dim_y;
    f32 *base = alloc(sizeof(f32) * cell_count, 7);
    return (Fluid) {
        .cell_size = cell_size,
        .cell_count = cell_count,
        .grid_dim_x = dim_x,
        .grid_dim_y = dim_y,
        .s  = base + cell_count*0,
        .u1 = base + cell_count*1,
        .u2 = base + cell_count*2,
        .v1 = base + cell_count*3,
        .v2 = base + cell_count*4,
        .m1 = base + cell_count*5,
        .m2 = base + cell_count*6,
    };
}

void Fluid_Reset(Fluid *fluid)
{
    for_n(i,0,fluid->cell_count)
    {
        fluid->s[i] = 1.0f;
        fluid->u1[i] = 0.0f;
        fluid->u2[i] = 0.0f;
        fluid->v1[i] = 0.0f;
        fluid->v2[i] = 0.0f;
        fluid->m1[i] = 0.0f;
        fluid->m2[i] = 0.0f;
    }
    for_n(x, 0, fluid->grid_dim_x)
    {
        get(s, x, 0) = 0.0f;
        get(s, x, fluid->grid_dim_y - 1) = 0.0f;
    }
    for_n(y, 0, fluid->grid_dim_y)
    {
        get(s, 0, y) = 0.0f;
        get(s, fluid->grid_dim_x - 1, y) = 0.0f;
    }
}

void Fluid_Clear(Fluid *fluid)
{
    fluid->cell_size = 0.0f;
    for_n(i,0,fluid->cell_count*7)
    {
        fluid->s[i] = 0.0f;
    }
}

void Fluid_Set(Fluid *fluid, Keyframe *frame)
{
    assert(IsValidKeyframe(fluid, frame));
    for_n(y, 0, frame->dim_y)
    for_n(x, 0, frame->dim_x)
    {
        get(m1, x+1, y+1) = frame->values[y*frame->dim_x+x];
    }
}

void Fluid_Dimensions(Fluid *fluid, int *width, int *height)
{
    *width = (f32)(fluid->grid_dim_x) * fluid->cell_size;
    *height = (f32)(fluid->grid_dim_y) * fluid->cell_size;
}

void Interact(Fluid *fluid, Interection_Type type, Vector2 position, f32 radius, f32 dx, f32 dy)
{
    f32 h = fluid->cell_size;
    for_n(y, 1, fluid->grid_dim_y-1)
    for_n(x, 1, fluid->grid_dim_x-1)
    {
        Vector2 cell_center = { (f32)(x)*h + h*0.5f, (f32)(y)*h + h*0.5f };
        
        f32 t = Vector2Distance(cell_center, position) / radius;
        if (get(s, x, y) != 0.0f && t < 1.0f)
        {
            f32 strength = 1.0 - t * t;
            switch (type) {
            break;case Interact_Smoke:
            {
                get(m1, x, y) += strength * 0.15;
            }
            break;case Interact_Velocity:
            {
                get(u1, x, y) += strength * 20 * dx;
                get(v1, x, y) += strength * 20 * dy;
            }
            break;case Interact_Smoke_And_Velocity:
            {
                get(u1, x, y) += strength * 20 * dx;
                get(v1, x, y) += strength * 20 * dy;
                get(m1, x, y) += strength * 0.15;
            }
            break;case Interact_Obstacle:
            {
                get(s, x, y) = 0.0;
                get(m1, x, y) = 0;
                get(u1, x, y) = 0.0;
                get(v1, x, y) = 0.0;
            }
            }
        }
    }
}

void Fluid_Draw(Fluid *fluid, Wind_Force *forces, int force_count, f32 t)
{
    f32 h = fluid->cell_size;
    for_n(y, 0, fluid->grid_dim_y)
    for_n(x, 0, fluid->grid_dim_x)
    {
        f32 force_strength = 0.0f;
        for_n(i, 0, force_count)
        {
            f32 cell_center_x = (f32)(x)*h + h*0.5f;
            f32 cell_center_y = (f32)(y)*h + h*0.5f;
            f32 dx = cell_center_x - forces[i].x;
            f32 dy = cell_center_y - forces[i].y;
            f32 r2 = dx*dx + dy*dy;
            f32 space_magnitude = expf(-1.0f * r2 / (forces[i].radius * forces[i].radius));
            f32 dt = t - forces[i].t;
            f32 time_magnitude = expf(-1.0f * dt * dt / (forces[i].interval * forces[i].interval));
            force_strength += forces[i].strength * forces[i].strength * space_magnitude * time_magnitude;
        }

        Vector2 position = { (f32)(x) * h, (f32)(y) * h };
        Vector2 size = { h, h };
        if (get(s, x, y) == 0.0f)
        {
            DrawRectangleV(position, size, SKYBLUE);
            continue;
        }
        f32 value = get(m1, x, y);
        Color smoke = ColorAlpha(WHITE, value / (value + 1.0f));
        Color force = ColorAlpha(RED, force_strength * 0.01f);
        DrawRectangleV(position, size, ColorAlphaBlend(smoke, force, WHITE));
    }
}

void Camera_Update(Camera2D *camera)
{
    // Translate based on mouse right click
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector2 delta = GetMouseDelta();
        Vector2 dtarget = Vector2Scale(delta, -1.0f / camera->zoom);
        camera->target = Vector2Add(camera->target, dtarget);
    }

    // Zoom based on mouse wheel
    f32 wheel = GetMouseWheelMove();
    if (wheel != 0.0f)
    {
        // Get the world point that is under the mouse
        Vector2 mouse_world_position = GetScreenToWorld2D(GetMousePosition(), *camera);

        // Set the offset to where the mouse is
        camera->offset = GetMousePosition();

        // Set the target to match, so that the camera maps the world space point
        // under the cursor to the screen space point under the cursor at any zoom
        camera->target = mouse_world_position;

        // Zoom increment
        // Uses log scaling to provide consistent zoom speed
        f32 scale = 0.1f * wheel;
        camera->zoom = Clamp(expf(logf(camera->zoom)+scale), 0.125, 64.0);
    }
}

void Extrapolate(Fluid *fluid)
{
    for_n(x, 0, fluid->grid_dim_x)
    {
        get(u1, x, 0)                   = get(u1, x, 1);
        get(u1, x, fluid->grid_dim_y-1) = get(u1, x, fluid->grid_dim_y-2);
    }
    for_n(y, 0, fluid->grid_dim_y)
    {
        get(v1, 0, y)                   = get(v1, 1, y);
        get(v1, fluid->grid_dim_x-1, y) = get(v1, fluid->grid_dim_x-2, y);
    }
}

#define Fluid_Sample Fluid_Sample_U
#define field u1
#include "fluid_sample.inl"
#undef field
#undef Fluid_Sample

#define Fluid_Sample Fluid_Sample_V
#define field v1
#include "fluid_sample.inl"
#undef field
#undef Fluid_Sample

#define Fluid_Sample Fluid_Sample_M
#define field m1
#include "fluid_sample.inl"
#undef field
#undef Fluid_Sample

f32 Fluid_AverageU(Fluid *fluid, int x, int y)
{
    return 0.25f * (get(u1, x+1, y) + get(u1, x, y) + get(u1, x+1, y-1) + get(u1, x, y-1));
}

f32 Fluid_AverageV(Fluid *fluid, int x, int y)
{
    return 0.25f * (get(v1, x-1, y) + get(v1, x, y) + get(v1, x-1, y+1) + get(v1, x, y+1));
}

void AdvectVelocity(Fluid *fluid, f32 dt)
{
    copy(u2, u1);
    copy(v2, v1);

    f32 h = fluid->cell_size;

    for_n(y, 1, fluid->grid_dim_y)
    for_n(x, 1, fluid->grid_dim_x)
    {
        if (get(s, x, y) != 0.0f && get(s, x-1, y) != 0.0f && y < fluid->grid_dim_y - 1)
        {
            f32 vx = get(u1, x, y);
            f32 vy = Fluid_AverageV(fluid, x, y);
            f32 px = (f32)(x) * h - dt * vx;
            f32 py = (f32)(y) * h + h * 0.5f - dt * vy;
            get(u2, x, y) = Fluid_Sample_U(fluid, px, py);
        }
        if (get(s, x, y) != 0.0f && get(s, x, y-1) != 0.0f && x < fluid->grid_dim_x - 1)
        {
            f32 vx = Fluid_AverageU(fluid, x, y);
            f32 vy = get(v1, x, y);
            f32 px = (f32)(x) * h + h * 0.5f - dt * vx;
            f32 py = (f32)(y) * h - dt * vy;
            get(v2, x, y) = Fluid_Sample_V(fluid, px, py);
        }
    }

    copy(u1, u2);
    copy(v1, v2);
}

void AdvectSmoke(Fluid *fluid, f32 dt)
{
    copy(m2, m1);

    f32 h = fluid->cell_size;

    for_n(y, 1, fluid->grid_dim_y-1)
    for_n(x, 1, fluid->grid_dim_x-1)
    {
        if (get(s, x, y) != 0.0f && get(s, x, y-1) != 0.0f)
        {
            f32 u = 0.5f * (get(u1, x, y) + get(u1, x+1, y));
            f32 v = 0.5f * (get(v1, x, y) + get(v1, x, y+1));
            f32 px = (f32)(x) * h + 0.5f * h - dt * u;
            f32 py = (f32)(y) * h + 0.5f * h - dt * v;
            get(m2, x, y) = Fluid_Sample_M(fluid, px, py);
        }
    }

    copy(m1, m2);
}

void Project(Fluid *fluid, int iters, f32 dt)
{
    (void)dt;
    for_n(iter, 0, iters)
    for_n(y, 1, fluid->grid_dim_y-1)
    for_n(x, 1, fluid->grid_dim_x-1)
    {
        if (get(s, x, y) == 0.0f) {
            continue;
        }

        f32 sx0 = get(s, x-1, y);
        f32 sx1 = get(s, x+1, y);
        f32 sy0 = get(s, x, y-1);
        f32 sy1 = get(s, x, y+1);
        f32 s = sx0 + sx1 + sy0 + sy1;
        if (s == 0.0f) {
            continue;
        }

        f32 div = get(u1, x+1, y) - get(u1, x, y) + get(v1, x, y+1) - get(v1, x, y);
        f32 p = -1.9 * div / s;

        get(u1, x+0, y) -= sx0 * p;
        get(u1, x+1, y) += sx1 * p;
        get(v1, x, y+0) -= sy0 * p;
        get(v1, x, y+1) += sy1 * p;
    }
}

void Heat(Fluid *fluid)
{
    for_n(y, 1, fluid->grid_dim_y-1)
    for_n(x, 1, fluid->grid_dim_x-1)
    {
        if (get(s, x, y) != 0.0f && get(s, x, y-1) != 0.0f)
        {
            get(v1, x, y) -= HEAT_CONSTANT * get(m1, x, y);
        }
    }
}

void ApplyControl(Fluid *fluid, Wind_Force *controls, int control_count, f32 t)
{
    f32 h = fluid->cell_size;

    for_n(y, 2, fluid->grid_dim_y-2)
    for_n(x, 2, fluid->grid_dim_x-2)
    for_n(i, 0, control_count)
    {
        f32 cell_center_x = (f32)(x)*h + h*0.5f;
        f32 cell_center_y = (f32)(y)*h + h*0.5f;
        f32 dx = cell_center_x - controls[i].x;
        f32 dy = cell_center_y - controls[i].y;
        f32 r2 = dx*dx + dy*dy;
        f32 dt = controls[i].t - t;

        // How strong the gaussian is in space
        f32 space_magnitude = expf(-1.0f * r2 / (controls[i].radius * controls[i].radius));

        // How strong the gaussian is in time
        f32 time_magnitude = expf(-1.0f * dt * dt / (controls[i].interval * controls[i].interval));

        f32 dir_x = cosf(controls[i].theta);
        f32 dir_y = sinf(controls[i].theta);
        f32 acc_x = controls[i].strength * controls[i].strength * dir_x * space_magnitude * time_magnitude;
        f32 acc_y = controls[i].strength * controls[i].strength * dir_y * space_magnitude * time_magnitude;

        // Add to velocity field
        get(u1, x, y) += acc_x;
        get(v1, x, y) += acc_y;
    }
}

void Fluid_Update(Fluid *fluid, Wind_Force *controls, int control_count, f32 t, f32 dt)
{
    ApplyControl(fluid, controls, control_count, t);
    Extrapolate(fluid);
    AdvectVelocity(fluid, dt);
    AdvectSmoke(fluid, dt);
    Heat(fluid);
    Project(fluid, PROJECTION_ITERATIONS, dt);
}

f32 MatchKeyframe(Fluid *fluid, Wind_Force *controls, int control_count, Keyframe *frame, f32 t, f32 final_t)
{
    f32 data_term = 0.0f;
    f32 regularization_term = 0.0f;
    for_n(y, 0, frame->dim_y)
    for_n(x, 0, frame->dim_x)
    {
        f32 target = frame->values[y*frame->dim_x+x];
        f32 real = get(m1, x+1, y+1);
        f32 image_difference = real - target;
        f32 image_contribution = image_difference * image_difference;
        data_term += image_contribution;
    }
    data_term *= expf((t - final_t) * 3.0f); // NOTE: 2 can be adjusted
    for_n(i, 0, control_count)
    {
        f32 volume = controls[i].radius * controls[i].radius * controls[i].interval * controls[i].interval;
        f32 control_contribution = CONTROL_PENALTY * volume * controls[i].strength * controls[i].strength;
        regularization_term += control_contribution;
    }
    //LOG("t = %.4f, data = %.5f\n", t, data_term);
    //LOG("t = %.4f, reg. = %.5f\n", t, regularization_term);
    //LOG("%.4f, %.5f\n", t, data_term);
    ACCUMULATE_DATA(data_term);
    ACCUMULATE_REGULARIZATION(regularization_term);
    return data_term + regularization_term;
}

#if 0
void MatchKeyframe_Simplified(float *objective, Wind_Force *controls, int control_count, float t)
{
    for_n(i, 0, control_count)
    {
        f32 volume = (f32)(M_PI) * controls[i].strength * controls[i].radius;
        f32 dt = t - controls[i].t;
        f32 time_magnitude = expf(-1.0f * dt * dt / controls[i].interval);
        f32 control_contribution = CONTROL_PENALTY * time_magnitude * volume;
        *objective += control_contribution;
    }
}

void __enzyme_autodiff_MatchKeyframe_Simplified(void *fn,
    float *objective, float *d_objective, Wind_Force *controls, Wind_Force *d_controls, int control_count,
    float t);

void rev_MatchKeyframe_Simplified(float *objective, Wind_Force *controls, Wind_Force *d_controls, int control_count, float t)
{
    float d_objective = 1.0f;
    float d_t = 0.0f;
    __enzyme_autodiff_MatchKeyframe_Simplified((void*)(MatchKeyframe_Simplified),
        objective, &d_objective,
        controls, d_controls,
        control_count,
        t
    );
}
#endif

void Fluid_ControlMethod(f32 *objective, Fluid *fluid, Wind_Force *controls, int control_count, Keyframe *frame, Parameters* parameters)
{
    f32 t = 0.0f;
    for (int i = 0; i < parameters->num_timesteps; ++i)
    {
        ApplyControl(fluid, controls, control_count, t);
        AdvectVelocity(fluid, parameters->step_size);
        AdvectSmoke(fluid, parameters->step_size);
        Heat(fluid);
        Project(fluid, PROJECTION_ITERATIONS, parameters->step_size);
        *objective += MatchKeyframe(fluid, controls, control_count, frame, t, parameters->final_t);
        t += parameters->step_size;
    }
}

void rev_Fluid_ControlMethod(f32 *objective, Fluid *fluid, Fluid *d_fluid, Wind_Force *controls, Wind_Force *d_controls, int control_count, Keyframe *frame, Parameters* parameters)
{
    f32 d_objective = 1.0f;
    __enzyme_autodiff_void((void*)(Fluid_ControlMethod),
        enzyme_dup, objective, &d_objective,
        enzyme_dup, fluid, d_fluid,
        enzyme_dup, controls, d_controls,
        enzyme_const, control_count,
        enzyme_const, frame,
        enzyme_const, parameters
    );
    (void)d_fluid;
}

void fwd_Fluid_ControlMethod(f32 *objective, f32 *d_objective, Fluid *fluid, Fluid *d_fluid, Wind_Force *controls, Wind_Force *d_controls, int control_count, Keyframe *frame, Parameters* parameters)
{
    __enzyme_fwddiff((void*)(Fluid_ControlMethod),
        enzyme_dup, objective, d_objective,
        enzyme_dup, fluid, d_fluid,
        enzyme_dup, controls, d_controls,
        enzyme_const, control_count,
        enzyme_const, frame,
        enzyme_const, parameters
    );
}

void SaveControls(const char *filename, Wind_Force *controls, size_t count)
{
    FILE *f = fopen(filename, "wb");
    assert(f != NULL);
    assert(fwrite(&count, sizeof(count), 1, f) == 1);
    assert(fwrite(controls, sizeof(Wind_Force), count, f) == count);
    assert(fclose(f) == 0);
    info("[%s] Saved controls (count = %zu)", filename, count);
}

Wind_Force *LoadControls(const char *filename, int *out_count)
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

Wind_Force *RandomControls(Fluid *fluid, int count, uint32_t seed)
{
    srand(seed);
    Wind_Force *forces = malloc(count * sizeof(Wind_Force));
    for_n(i, 0, count)
    {
        forces[i] = (Wind_Force) {
            .x        = random_range_f32(200.0f, 270.0f),
            .y        = random_range_f32(270.0f, 200.0f),
            .radius   = 5.0f,
            .theta    = random_f32() * 2.0f * (f32)(M_PI),
            .strength = 3.0f,
            .t        = random_range_f32(0.7f, 0.6f),
            .interval = random_range_f32(1.0f, 0.05f),
        };
    }
    return forces;
}

Wind_Force *ZeroControls(int count)
{
    return calloc(count, sizeof(Wind_Force));
}

void Controls_Clear(Wind_Force *controls, int count)
{
    for_n(i, 0, count)
    {
        controls[i] = (Wind_Force) {0};
    }
}

void Controls_Update(Wind_Force *controls, Wind_Force *d_controls, int count, float c)
{
    for_n(i, 0, count)
    {
        // Check NaNs
        assert(!isnan(d_controls[i].x));
        assert(!isnan(d_controls[i].y));
        assert(!isnan(d_controls[i].radius));
        assert(!isnan(d_controls[i].strength));
        assert(!isnan(d_controls[i].interval));
        assert(!isnan(d_controls[i].t));
        assert(!isnan(d_controls[i].theta));

        // Move along gradient
        controls[i].x        += 1.0f * c * d_controls[i].x;
        controls[i].y        += 1.0f * c * d_controls[i].y;
        controls[i].radius   += 1.0f * c * d_controls[i].radius;
        controls[i].strength += 1.0f * c * d_controls[i].strength;
        controls[i].interval += 1.0f * c * d_controls[i].interval;
        controls[i].t        += 1.0f * c * d_controls[i].t;
        controls[i].theta    += 1.0f * c * d_controls[i].theta;
    }
}

Adam Adam_InitAndClear(int count)
{
    return (Adam) {
        .mean     = ZeroControls(count),
        .velocity = ZeroControls(count),
    };
}

// * Gradient Decent with momentum *
// alpha = learning rate
// beta1 = decay rate for mean
// beta2 = decay rate for mean squared
void Controls_UpdateAdam(Wind_Force *controls, Wind_Force *d_controls, int count, int iter, Adam *adam, float alpha, float beta1, float beta2)
{
    int array_count = 0;
    f32 *w = Controls_ToFloatArray(controls, count, &array_count);
    f32 *g = Controls_ToFloatArray(d_controls, count, 0);
    f32 *m = Controls_ToFloatArray(adam->mean, count, 0);
    f32 *v = Controls_ToFloatArray(adam->velocity, count, 0);
    f32 t = (f32)(iter);

    const float epsilon = 1e-7f;
    for_n(i, 0, array_count)
    {
        // Check NaN
        assert(!isnan(w[i]));
        assert(!isnan(g[i]));
        assert(!isnan(m[i]));
        assert(!isnan(v[i]));

        // Adam
        m[i] = beta1 * v[i] + (1.0f - beta1) * g[i];
        v[i] = beta2 * v[i] + (1.0f - beta2) * g[i] * g[i];
        
        // Bias correction
        m[i] = m[i] / (1.0f - powf(beta1, t));
        v[i] = v[i] / (1.0f - powf(beta2, t));
        
        // Update
        w[i] = w[i] - (m[i] / (sqrtf(v[i]) + epsilon)) * alpha;
    }
}

void Controls_Copy(Wind_Force *dst_controls, Wind_Force *src_controls, int count)
{
    for_n(i, 0, count)
    {
        dst_controls[i] = src_controls[i];
    }
}

void Controls_FromDoubleArray(Wind_Force *dst_controls, double const *src, int count)
{
    int count_f32 = 0;
    f32 *array_f32 = Controls_ToFloatArray(dst_controls, count, &count_f32);
    for_n(i, 0, count)
    {
        array_f32[i] = (f32)(src[i]);
    }
}

double *Controls_ToDoubleArray(Wind_Force *controls, int count)
{
    int count_f32 = 0;
    f32 *array_f32 = Controls_ToFloatArray(controls, count, &count_f32);
    double *dst = alloc(sizeof(double), count_f32);
    for_n(i, 0, count)
    {
        dst[i] = (double)(array_f32[i]);
    } 
    return dst;
}

void Controls_SetBounds(nlopt_opt opt, Fluid *fluid, Parameters *parameters, int control_count)
{
    int const values_per_control = sizeof(Wind_Force) / sizeof(f32);
    for_n(i, 0, control_count)
    {
        // x
        nlopt_set_lower_bound(opt, i*7+0, 0.0);
        nlopt_set_upper_bound(opt, i*7+0, (double)(fluid->cell_size) * (double)(fluid->grid_dim_x));
        // y
        nlopt_set_lower_bound(opt, i*7+1, 0.0);
        nlopt_set_upper_bound(opt, i*7+1, (double)(fluid->cell_size) * (double)(fluid->grid_dim_y));
        // radius
        nlopt_set_lower_bound(opt, i*7+2, 0.0);
        nlopt_set_upper_bound(opt, i*7+2, 100.0);
        // theta
        nlopt_set_lower_bound(opt, i*7+3, 0.0);
        nlopt_set_upper_bound(opt, i*7+3, 2 * M_PI);
        // strength
        nlopt_set_lower_bound(opt, i*7+4, 0.0);
        nlopt_set_upper_bound(opt, i*7+4, 100.0);
        // t
        nlopt_set_lower_bound(opt, i*7+5, 0.0);
        nlopt_set_upper_bound(opt, i*7+5, parameters->final_t);
        // interval
        nlopt_set_lower_bound(opt, i*7+6, 0.0);
        nlopt_set_upper_bound(opt, i*7+6, 2.0);
    }
}

Keyframe LoadKeyframe(const char *filename)
{
    Image image = LoadImage(filename);
    assert(image.data != NULL);

    Keyframe frame = {
        .dim_x = image.width,
        .dim_y = image.height,
        .values = malloc(sizeof(f32) * image.width * image.height),
    };

    for_n(y, 0, frame.dim_y)
    for_n(x, 0, frame.dim_x)
    {
        Vector4 pixel = ColorNormalize(GetImageColor(image, x, y));
        f32 value = pixel.w / (1.0f - pixel.w);
        frame.values[y*frame.dim_x+x] = value;
    }

    UnloadImage(image);
    return frame;
}

void Keyframe_Draw(Keyframe *frame, f32 h)
{
    for_n(y, 0, frame->dim_y)
    for_n(x, 0, frame->dim_x)
    {
        Vector2 position = { (f32)(x + 1) * h, (f32)(y + 1) * h };
        Vector2 size = { h, h };
        f32 value = frame->values[y*frame->dim_x+x];
        Color smoke = ColorAlpha(WHITE, value / (value + 1.0f));
        DrawRectangleV(position, size, smoke);
    }
}

int test_finitediff(void)
{
    Fluid fluid = Fluid_Create(4.0f, 100, 100);
    Fluid d_fluid = Fluid_Create(4.0f, 100, 100);
    Keyframe initial_keyframe = LoadKeyframe("initial.smoke.png");
    Keyframe target_keyframe = LoadKeyframe("target.smoke.png");
    Parameters parameters = {
        .num_timesteps = 100,
        .step_size = 0.02f,
        .final_t = (f32)(100) * 0.02f,
    };

    int control_count = 100;
    Wind_Force *random_controls = RandomControls(&fluid, control_count, 0x6767);
    Wind_Force *controls = ZeroControls(control_count);
    Wind_Force *d_controls = ZeroControls(control_count);

    struct {
        int offset;
        const char *name;
    } members[7] = {
        { 0, "x" },
        { 1, "y" },
        { 2, "radius" },
        { 3, "theta" },
        { 4, "strength" },
        { 5, "t" },
        { 6, "interval" },
    };

    f32 real_objective = 0.0f;
    for (int i = 0; i < 7; ++i)
    {
        Fluid_Reset(&fluid);
        Fluid_Set(&fluid, &initial_keyframe);
        Controls_Copy(controls, random_controls, control_count);
        
        f32 epsilon = 0.000001f;
        f32 objective0 = 0.1f;
        Fluid_ControlMethod(&objective0, &fluid, controls, control_count, &target_keyframe, &parameters);
        f32 objective1 = 0.0f;
        *((f32*)(controls) + members[i].offset) += epsilon;
        Fluid_Reset(&fluid);
        Fluid_Set(&fluid, &initial_keyframe);
        Fluid_ControlMethod(&objective1, &fluid, controls, control_count, &target_keyframe, &parameters);
        f32 finite_difference = (objective1 - objective0) / epsilon;
        if (i==0) { real_objective = objective0; printf("objective = %.5f\n", real_objective); }
        assert(objective0 == real_objective);
        printf("(FD)  d_controls[0].%s = %.5f\n", members[i].name, finite_difference);
    }
    {
        Fluid_Reset(&fluid);
        Fluid_Clear(&d_fluid);
        Fluid_Set(&fluid, &initial_keyframe);
        Controls_Clear(d_controls, control_count);
        Controls_Copy(controls, random_controls, control_count);

        f32 objective = 0.0f;
        rev_Fluid_ControlMethod(
            &objective, &fluid, &d_fluid, controls, d_controls, control_count, &target_keyframe, &parameters
        );
        assert(objective == real_objective);
        printf("(REV) d_controls[0].x = %.5f\n", d_controls[0].x);
        printf("(REV) d_controls[0].y = %.5f\n", d_controls[0].y);
        printf("(REV) d_controls[0].radius = %.5f\n", d_controls[0].radius);
        printf("(REV) d_controls[0].theta = %.5f\n", d_controls[0].theta);
        printf("(REV) d_controls[0].strength = %.5f\n", d_controls[0].strength);
        printf("(REV) d_controls[0].t = %.5f\n", d_controls[0].t);
        printf("(REV) d_controls[0].interval = %.5f\n", d_controls[0].interval);
    }
    for (int i = 0; i < 7; ++i)
    {
        Fluid_Reset(&fluid);
        Fluid_Clear(&d_fluid);
        Fluid_Set(&fluid, &initial_keyframe);
        Controls_Clear(d_controls, control_count);
        Controls_Copy(controls, random_controls, control_count);
        
        f32 objective = 0.0f;
        f32 d_objective = 0.0f;
        *((f32*)(d_controls) + members[i].offset) = 1.0f;
        fwd_Fluid_ControlMethod(
            &objective, &d_objective, &fluid, &d_fluid, controls, d_controls, control_count, &target_keyframe, &parameters
        );
        assert(objective == real_objective);
        printf("(FWD) d_controls[0].%s = %.5f\n", members[i].name, d_objective);
    }

    return 0;
}

int test_run(void)
{
    Fluid fluid = Fluid_Create(4.0f, 100, 100);
    Keyframe initial_keyframe = LoadKeyframe("initial.smoke.png");
    Keyframe target_keyframe = LoadKeyframe("target.smoke.png");
    Parameters parameters = {
        .num_timesteps = 100,
        .step_size = 0.02f,
        .final_t = (f32)(100) * 0.02f,
    };

    int control_count = 100;
    Wind_Force *controls = RandomControls(&fluid, control_count, 0x6767);

    Fluid_Reset(&fluid);
    Fluid_Set(&fluid, &initial_keyframe);
    f32 objective = 0.0f;
    Fluid_ControlMethod(&objective, &fluid, controls, control_count, &target_keyframe, &parameters);
    printf("objective = %.5f\n", objective);
    printf("data term = %.5f\n", global_data_term);
    printf("regularization term = %.5f\n", global_regularization_term);
    return 0;
}

int test(const char *name)
{
    if (0);
    else if (strcmp(name, "finitediff") == 0)
        return test_finitediff();
    else if (strcmp(name, "run") == 0)
        return test_run();
    return 1;
}

typedef struct {
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

double Fluid_Objective(unsigned n, const double *x, double *grad, void *data)
{
    Fluid_Objective_Data *d = (Fluid_Objective_Data*) data;
    float objective = 0.0f;
    Controls_FromDoubleArray(d->controls, x, d->control_count);
    SaveControls(TextFormat("int/optimized_%i.controls", d->iter++), d->controls, d->control_count);
    if (grad) {
        Fluid_Reset(&d->fluid);
        Fluid_Reset(&d->d_fluid);
        Fluid_Set(&d->fluid, &d->initial);
        Controls_Clear(d->d_controls, d->control_count);
        rev_Fluid_ControlMethod(
            &objective,
            &d->fluid, &d->d_fluid,
            d->controls, d->d_controls, d->control_count,
            &d->target, &d->parameters
        );
        int count = 0;
        f32 *gf = Controls_ToFloatArray(d->d_controls, d->control_count, &count);
        assert((unsigned)(count) == n);
        for (unsigned i = 0; i < n; ++i)
        {
            grad[i] = (double)(gf[i]);
        }
    } else {
        Fluid_Reset(&d->fluid);
        Fluid_Set(&d->fluid, &d->initial);
        Fluid_ControlMethod(
            &objective, &d->fluid, d->controls, d->control_count,
            &d->target, &d->parameters
        );
    }
    printf("objective = %.5f\n", objective);
    return (double)(objective);
}

int main(int argc, char *argv[])
{
    // Parse command line
    int optimize = 0;
    uint32_t seed = 0x6767;
    int stop_at_final_timestep = 0;
    char *control_filename = 0;
    for (int i = 1; i < argc; ++i) if (0);
        else if (strcmp(argv[i], "stop") == 0)
            stop_at_final_timestep = 1;
        else if (strcmp(argv[i], "seed") == 0)
            seed = strtoul(argv[++i], 0, 10);
        else if (strcmp(argv[i], "opt") == 0)
            optimize = 1;
        else if (strcmp(argv[i], "load") == 0 && i + 1 < argc)
            control_filename = argv[++i];
        else if (strcmp(argv[i], "test") == 0 && i + 1 < argc)
            return test(argv[++i]);

    Fluid fluid = Fluid_Create(4.0f, 100, 100);
    Fluid d_fluid = Fluid_Create(4.0f, 100, 100);
    if (!optimize)
    {
        int width, height;
        Fluid_Dimensions(&fluid, &width, &height);
        InitWindow(width, height, __FILE__);
    }
    Fluid_Reset(&fluid);

    int control_count = 100;
    Wind_Force *controls;

    if (control_filename)
        controls = LoadControls(control_filename, &control_count);
    else
        controls = RandomControls(&fluid, control_count, seed);

    Keyframe initial_keyframe = LoadKeyframe("initial.smoke.png");
    Keyframe target_keyframe = LoadKeyframe("target.smoke.png");
    Fluid_Set(&fluid, &initial_keyframe);

    Parameters parameters = {
        .num_timesteps = 100,
        .step_size = 0.02f,
        .final_t = (f32)(100) * 0.02f,
    };

    if (optimize)
    {
        SaveControls("initial.controls", controls, control_count);
        Wind_Force *d_controls = ZeroControls(control_count);
        Adam adam = Adam_InitAndClear(control_count);

        printf("Begin optimization!\n");

        {
            Fluid_Objective_Data data = {
                .fluid = fluid,
                .d_fluid = d_fluid,
                .controls = controls,
                .d_controls = d_controls,
                .control_count = control_count,
                .initial = initial_keyframe,
                .target = target_keyframe,
                .parameters = parameters,
            };

            unsigned int n = (unsigned int)(control_count) * sizeof(Wind_Force) / sizeof(float);    
            nlopt_opt opt = nlopt_create(NLOPT_LD_LBFGS, n); /* algorithm and dimensionality */
            nlopt_set_min_objective(opt, Fluid_Objective, &data);
            
            Controls_SetBounds(opt, &fluid, &parameters, control_count);

            double *x = Controls_ToDoubleArray(controls, control_count);
            double minf;
            int result = 0;
            if ((result = nlopt_optimize(opt, x, &minf)) < 0) {
                printf("nlopt failed! (%i)\n", result);
            }
            else {
                printf("found minimum at f(x) = %0.10g\n", minf);
                Controls_FromDoubleArray(controls, x, control_count);
                SaveControls("optimized_nlopt.controls", controls, control_count);
            }
            printf("done\n");
            exit(0);
        }

        for (int iter = 1; iter <= 2000; iter++)
        {
            // Setup initial state
            Fluid_Reset(&fluid);
            Fluid_Reset(&d_fluid);
            Fluid_Set(&fluid, &initial_keyframe);
            Controls_Clear(d_controls, control_count);
#if 0
            SaveControls("initial.controls", controls, 1);
            f32 obj1 = 0.0f;
            MatchKeyframe_Simplified(&obj1, controls, 1, 2.0f);
            f32 obj2 = 0.0f;
            rev_MatchKeyframe_Simplified(&obj2, controls, d_controls, 1, 2.0f);
            SaveControls("grad.controls", d_controls, 1);
            printf("obj2 = %.5f =? %.5f\n", obj1, obj2);
            exit(1);
#endif      

            // Solve for gradient
            f32 objective = 0.0f;
            rev_Fluid_ControlMethod(
                &objective, &fluid, &d_fluid, controls, d_controls, control_count, &target_keyframe, &parameters
            );
            //Fluid_ControlMethod(&objective, &fluid, controls, control_count, &target_keyframe, &parameters);

            // Display current progress
            printf("objective = %.1f\n", objective);

            if (iter == 1) {
                SaveControls("grad.controls", d_controls, control_count);
            }

            // Update controls
            Controls_Update(controls, d_controls, control_count, -0.00025f);
            //Controls_UpdateAdam(controls, d_controls, control_count, iter, &adam, 0.0000001f, 0.9f, 0.999f);

            const char* optimized_filename = TextFormat("optimized_%i.controls", iter);

            // Write out new controls
            SaveControls(optimized_filename, controls, control_count);
            // SaveControls("grad.controls", d_controls, control_count);
            //exit(0);
        }
    }

    Camera2D camera = { .zoom = 1 };
    Font font = LoadFontEx("GoogleSansCode-Regular.ttf", 64, 0, 0);
    Vector2 mouse_position = GetScreenToWorld2D(GetMousePosition(), camera);
    Interection_Type interact_type = Interact_Smoke;
    bool simulate = false;
    f32 t = 0.0f;
    int step = 0;

    while (!WindowShouldClose())
    {
        Vector2 mouse_position_old = mouse_position;
        mouse_position = GetScreenToWorld2D(GetMousePosition(), camera);
        Camera_Update(&camera);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            f32 dx = mouse_position.x - mouse_position_old.x;
            f32 dy = mouse_position.y - mouse_position_old.y;
            Interact(&fluid, interact_type, mouse_position, INTERACTION_RADIUS, dx, dy);
        }
        if (IsKeyPressed(KEY_R))
        {
            Fluid_Reset(&fluid);
            Fluid_Set(&fluid, &initial_keyframe);
            t = 0;
            step = 0;
        }
        if (IsKeyPressed(KEY_SPACE)) { simulate = !simulate; }
        if (IsKeyPressed(KEY_ONE))   { interact_type = Interact_Smoke; }
        if (IsKeyPressed(KEY_TWO))   { interact_type = Interact_Velocity; }
        if (IsKeyPressed(KEY_THREE)) { interact_type = Interact_Smoke_And_Velocity; }
        if (IsKeyPressed(KEY_FOUR))  { interact_type = Interact_Obstacle; }
        if (simulate)
        {
            f32 dt = parameters.step_size;
            Fluid_Update(&fluid, controls, control_count, t, dt);
            t += dt;
            step += 1;

            if (stop_at_final_timestep && step == parameters.num_timesteps)
            {
                simulate = false;
            }
        }
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);
        Fluid_Draw(&fluid, controls, control_count, t);
        if (IsKeyDown(KEY_K)) { Keyframe_Draw(&target_keyframe, fluid.cell_size); }
        EndMode2D();
        const char *text = TextFormat("%i FPS (%.3f) step = %i", GetFPS(), t, step);
        DrawTextEx(font, text, (Vector2){5, 5}, 16, 1.0f, BLACK);
        DrawTextEx(font, text, (Vector2){4, 4}, 16, 1.0f, RAYWHITE);
        EndDrawing();
    }
}
