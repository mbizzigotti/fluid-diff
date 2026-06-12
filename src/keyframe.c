// Generate keyframes from code.

#include "raylib.h"
#include "raymath.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

float tonemap(float x)
{
    return x / (x + 1);
}

float inv_tonemap(float x)
{
    return -x / (x - 1);
}

Image image;
Font font;

#define match(s) (strcmp(argv[i], s) == 0)
#define KEYFRAME_DIR "configs/keyframes"

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1, 1, "");
    const char *font_file = "assets/GoogleSansCode-Bold.ttf";
    
    for (int i = 1; i < argc; ++i)
    {
        if match("font")
        {
            font_file = argv[++i];
        }
        if match("test")
        {
            image = GenImageColor(100, 100, ColorAlpha(BLACK, 0.0f));
            ImageDrawRectangle(&image, 45, 85, 10, 10, ColorAlpha(WHITE, tonemap(2.0f)));
            ExportImage(image, KEYFRAME_DIR"/test_initial.smoke.png");
            
            image = GenImageColor(100, 100, ColorAlpha(BLACK, 0.0f));
            ImageDrawRectangle(&image, 15, 55, 70, 10, ColorAlpha(WHITE, tonemap(2.0f/6.0f)));
            ExportImage(image, KEYFRAME_DIR"/test_final.smoke.png");
        }
        if match("text")
        {
            char *text = argv[++i];
            int height = strtol(argv[++i], 0, 10);

            int count = 0;
            int *codepoints = LoadCodepoints(text, &count);
            int font_size = 96;
            font = LoadFontEx(font_file, font_size, codepoints, count);
            Vector2 size = MeasureTextEx(font, text, (float)(font_size), 1.0f);
            int width = (int)(size.x) + 10;

            const char *output;
            //image = GenImageColor(width, height, ColorAlpha(BLACK, 0.0f));
            //int left = (width - 10) / 2;
            //int top = height - 10 - 5;
            //ImageDrawRectangle(&image, left, top, 10, 5, ColorAlpha(WHITE, tonemap(4.0f)));
            //output = TextFormat(KEYFRAME_DIR"/%s_initial.smoke.png", text);
            //ExportImage(image, output);
            
            Vector2 image_size = { (float)(width), (float)(height) };
            image = GenImageColor(width, height, ColorAlpha(BLACK, 0.0f));
            printf("INFO: SIZE: %fx%f\n", size.x, size.y);
            Vector2 position = Vector2Scale(Vector2Subtract(image_size, size), 0.5f);
            ImageDrawTextEx(&image, font, text, position, (float)(font_size), 1.0f, ColorAlpha(WHITE, tonemap(2.0f/12.0f)));
            output = TextFormat(KEYFRAME_DIR"/%s_final.smoke.png", text);
            ExportImage(image, output);
        }
        if match("tzumaoli")
        {
            image = GenImageColor(120, 85, ColorAlpha(BLACK, 0.0f));
            ImageDrawCircle(&image, 60, 60, 6, ColorAlpha(WHITE, tonemap(20.0f)));
            ExportImage(image, KEYFRAME_DIR"/ball.smoke.png");
        }
    }
}
