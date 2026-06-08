#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WIDTH                  100
#define HEIGHT                 100
#define CELL_SIZE              4
#define STEP_SIZE              (1.0f/120.0f)
#define PROJECTION_ITERATIONS  100
#define HEAT_CONSTANT          6.0f
#define INTERACTION_RADIUS     20
#define CONTROL_PENALTY        1.0f

float __enzyme_autodiff(void*, ...);

typedef struct {
    float a1, a2, a3, a4, a5, a6, a7;
} Thing;

#if 0
float sum_array(Thing *arr, int count)
{
    float objective = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        float volume = (float)(M_PI) * arr[i].a5 * arr[i].a3;
        float dt = 1.234f - arr[i].a6;
        float time_magnitude = expf(-1.0f * dt * dt / arr[i].a7);
        float control_contribution = CONTROL_PENALTY * time_magnitude * volume;
        objective += control_contribution;
    }
    return objective;
}
float rev_sum_array(Thing *arr, Thing *d_arr, int count)
{
    return __enzyme_autodiff((void*)(sum_array), arr, d_arr, count);
}
int main(int argc, char *argv[])
{
    #define T(X) {X,X,X,X,X,X,X}
    Thing a[]  = {T(1.0f), T(2.0f), T(3.0f), T(4.0f)};
    Thing b[4] = {T(0.0f), T(0.0f), T(0.0f), T(0.0f)};

    if (argc == 5) {
        a[0] = (Thing) T(strtof(argv[1], 0));
        a[1] = (Thing) T(strtof(argv[2], 0));
        a[2] = (Thing) T(strtof(argv[3], 0));
        a[3] = (Thing) T(strtof(argv[4], 0));
    }

    float test1 = sum_array(a, 4);
    float test2 = rev_sum_array(a, b, 4);
    printf("MatchKeyframe_Simplified %.5f, %.5f\n", test1, test2);

    for (int i = 0; i < 4; ++i) {
        printf("arr[%i] = {%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f}, "
               "d_arr[%i] = {%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f}\n",
            i, a[i].a1, a[i].a2, a[i].a3, a[i].a4, a[i].a5, a[i].a6, a[i].a7,
            i, b[i].a1, b[i].a2, b[i].a3, b[i].a4, b[i].a5, b[i].a6, b[i].a7
        );
    }
}
#endif

#if 0
float sum_array(float *arr, int count)
{
    float objective = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        objective += arr[i] * arr[i];
    }
    return objective;
}
float rev_sum_array(float *arr, float *d_arr, int count)
{
    return __enzyme_autodiff((void*)(sum_array), arr, d_arr, count);
}
int main(int argc, char *argv[])
{
    float a[]  = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    if (argc == 5) {
        a[0] = strtof(argv[1], 0);
        a[1] = strtof(argv[2], 0);
        a[2] = strtof(argv[3], 0);
        a[3] = strtof(argv[4], 0);
    }

    float test1 = sum_array(a, 4);
    float test2 = rev_sum_array(a, b, 4);
    printf("MatchKeyframe_Simplified %.5f, %.5f\n", test1, test2);

    for (int i = 0; i < 4; ++i) {
        printf("arr[%i] = %.3f, d_arr[%i] = %.3f\n",
            i, a[i],
            i, b[i]
        );
    }
}
#endif

#if 1
float multiply(float a, float b)
{
    return a * b;
}
float rev_multiply(float a, float *da, float b, float *db)
{
    (void)sizeof(da, db);
    return __enzyme_autodiff((void*)(multiply), a, b);
}
int main(void)
{
    float a = 2.0f;
    float b = 3.0f;
    float da = 0.0f, db = 0.0f;

    float test1 = multiply(a, b);
    float test2 = rev_multiply(a, &da, b, &db);
    printf("MatchKeyframe_Simplified %.5f, %.5f\n", test1, test2);

    printf("da = %.3f, db = %.3f\n", da, db);
}
#endif
