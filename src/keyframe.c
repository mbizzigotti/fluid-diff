// Generate keyframes from code.

#include "raylib.h"
#include "raymath.h"
#include <string.h>

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

void single_letter(char ch, int width, int height)
{
    const char *output;
    image = GenImageColor(width, height, ColorAlpha(BLACK, 0.0f));
    int left = (width - 10) / 2;
    int top = height - 10 - 5;
    ImageDrawRectangle(&image, left, top, 10, 10, ColorAlpha(WHITE, tonemap(4.0f)));
    output = TextFormat(KEYFRAME_DIR"/letter%c_initial.smoke.png", ch);
    ExportImage(image, output);
    
    Vector2 image_size = { (float)(width), (float)(height) };
    float font_size = 96.0f;
    char text[2] = { ch, '\0' };
    image = GenImageColor(width, height, ColorAlpha(BLACK, 0.0f));
    Vector2 size = MeasureTextEx(font, text, font_size, 1.0f);
    Vector2 position = Vector2Scale(Vector2Subtract(image_size, size), 0.5f);
    ImageDrawTextEx(&image, font, text, position, font_size, 1.0f, ColorAlpha(WHITE, tonemap(2.0f/12.0f)));
    output = TextFormat(KEYFRAME_DIR"/letter%c_final.smoke.png", ch);
    ExportImage(image, output);
}

int main(int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1, 1, "");
    font = LoadFontEx("assets/GoogleSansCode-Bold.ttf", 96, 0, 0);
    
    for (int i = 1; i < argc; ++i)
    {
        if match("test")
        {
            image = GenImageColor(100, 100, ColorAlpha(BLACK, 0.0f));
            ImageDrawRectangle(&image, 45, 85, 10, 10, ColorAlpha(WHITE, tonemap(2.0f)));
            ExportImage(image, KEYFRAME_DIR"/test_initial.smoke.png");
            
            image = GenImageColor(100, 100, ColorAlpha(BLACK, 0.0f));
            ImageDrawRectangle(&image, 15, 55, 70, 10, ColorAlpha(WHITE, tonemap(2.0f/10.0f)));
            ExportImage(image, KEYFRAME_DIR"/test_final.smoke.png");
        }
        if match("letterc") single_letter('C', 60, 100);
        if match("letters") single_letter('S', 60, 100);
        if match("lettere") single_letter('E', 60, 100);
        if match("letter2") single_letter('2', 60, 100);
        if match("letter9") single_letter('9', 60, 100);
        if match("letter1") single_letter('1', 60, 100);
        if match("tzumaoli")
        {
            image = GenImageColor(120, 85, ColorAlpha(BLACK, 0.0f));
            ImageDrawCircle(&image, 60, 60, 6, ColorAlpha(WHITE, tonemap(20.0f)));
            ExportImage(image, KEYFRAME_DIR"/ball.smoke.png");
        }
    }
}
