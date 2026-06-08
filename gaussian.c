#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

typedef struct {
    float *values;
    int count;
    float min, max;
} DiscreteFunction;

typedef struct {
    float amplitude;
    float mean;
    float stddev;
} Gaussian;

float eval_gaussian(Gaussian *g, float x)
{
    float r = (x - g->mean) / g->stddev;
    return g->amplitude * expf(-r * r);
}

int enzyme_dup, enzyme_const;
float __enzyme_autodiff(void*, ...);

Camera2D CameraFromRectangle(float x, float y, float width)
{
    float dx = 0.5f * (float)(GetScreenWidth());
    float dy = 0.5f * (float)(GetScreenHeight());
    return (Camera2D) {
        .offset = { dx, dy },
        .target = { x, y },
        .zoom = (float)(GetScreenWidth()) / width,
    };
}

void DrawGridLines(float minor_step_size)
{
    float x = 0.0f;
    while (x < 1.0f)
    {
        x += minor_step_size;
        DrawLineEx((Vector2){x, -1.0f}, (Vector2){x, 1.0f}, 0.005f, ColorFromHSV(0.0f, 0.0f, 0.1f));
        DrawLineEx((Vector2){-x, -1.0f}, (Vector2){-x, 1.0f}, 0.005f, ColorFromHSV(0.0f, 0.0f, 0.1f));
        DrawLineEx((Vector2){-1.0f,  x}, (Vector2){1.0f,  x}, 0.005f, ColorFromHSV(0.0f, 0.0f, 0.1f));
        DrawLineEx((Vector2){-1.0f, -x}, (Vector2){1.0f, -x}, 0.005f, ColorFromHSV(0.0f, 0.0f, 0.1f));
    }
    DrawLine(0, -1, 0, 1, RAYWHITE);
    DrawLine(-1, 0, 1, 0, RAYWHITE);
}

void DrawFunction(DiscreteFunction *fn, Color color)
{
    for (int i = 1; i < fn->count; ++i)
    {
        float y0 = fn->values[i-1];
        float y1 = fn->values[i-0];
        float x0 = ((float)(i - 1) / (float)(fn->count - 1)) * (fn->max - fn->min) + fn->min;
        float x1 = ((float)(i) / (float)(fn->count - 1)) * (fn->max - fn->min) + fn->min;
        DrawLineEx((Vector2){x0, -y0}, (Vector2){x1, -y1}, 0.005f, color);
    }
}

void DrawGaussian(Gaussian *g, Color color)
{
    float x = -2.0f;
    while (x < 2.0f)
    {
        float x0 = x;
        float x1 = x + 0.01f;
        float y0 = eval_gaussian(g, x0);
        float y1 = eval_gaussian(g, x1);
        DrawLineEx((Vector2){x0, -y0}, (Vector2){x1, -y1}, 0.005f, color);
        x = x1;
    }
}

void loss(float *output, Gaussian *g, DiscreteFunction *fn)
{
    for (int i = 0; i < fn->count; ++i)
    {
        float y = fn->values[i];
        float x = ((float)(i) / (float)(fn->count - 1)) * (fn->max - fn->min) + fn->min;
        float d = eval_gaussian(g, x) - y;
        *output += d * d;
    }
}

void __enzyme_autodiff_loss(void *f, float *loss, float *d_loss, ...);

void rev_loss(float *out, Gaussian *g, Gaussian *dg, DiscreteFunction *fn)
{
    float d_loss = 1.0f;
    __enzyme_autodiff_loss((void*)(loss), out, &d_loss, g, dg, enzyme_const, fn);
}

int main(int argc, char *argv[])
{
    InitWindow(800, 600, __FILE__);
    SetTargetFPS(60);
    Font font = LoadFontEx("GoogleSansCode-Regular.ttf", 64, 0, 0);
    Camera2D camera = CameraFromRectangle(0.0f, 0.0f, 4.0f);

    float t = 0.0f;

    DiscreteFunction target;
    target.count = 64;
    target.values = malloc(sizeof(float) * target.count);
    target.min = -0.0f * 1.0f;
    target.max =  1.0f * 1.0f;

    for (int i = 0; i < target.count; ++i)
    {
        float t = (float)(i) / (float)(target.count - 1);
        float x = t * (target.max - target.min) + target.min;
        target.values[i] = (x > 0.1f && x < 0.9f) ? 1.0f : 0.0f;
        //target.values[i] = sinf(x*4.0f);
    }

    Gaussian g = {.amplitude = 1.0f, .mean = -0.5f, .stddev = 0.5f};

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
        Gaussian mouse_g = {.amplitude = 1.0f, .mean = mouse.x, .stddev = 0.5f};

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  { g = mouse_g; }

        BeginMode2D(camera);
        DrawGridLines(0.1f);
        DrawGaussian(&g, GREEN);
        DrawFunction(&target, BLUE);
        DrawGaussian(&mouse_g, ColorAlpha(WHITE, 0.25f));
        EndMode2D();

        float l = 0.0f;
        // loss(&l, &g, &target);

        Gaussian grad = {};
        {
            rev_loss(&l, &g, &grad, &target);
            g.amplitude += -0.001f * grad.amplitude;
            g.mean      += -0.001f * grad.mean;
            g.stddev    += -0.001f * grad.stddev;
        }

        const char *text = TextFormat(
            "grad = (amplitude = %.5f, mean = %.5f, stddev = %.5f)\n"
            "loss = %.5f",
            grad.amplitude, grad.mean, grad.stddev, l
        );
        DrawTextEx(font, text, (Vector2){4, 4}, 24, 1.0f, WHITE);
        EndDrawing();
    }
}
