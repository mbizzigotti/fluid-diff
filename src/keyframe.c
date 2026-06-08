// Generate keyframes from code.

#include "raylib.h"

float tonemap(float x)
{
    return x / (x + 1);
}

float inv_tonemap(float x)
{
    return -x / (x - 1);
}

int main(void)
{
    if (1)
    {
        Image image = GenImageColor(100, 100, ColorAlpha(BLACK, 0.0f));
        ImageDrawRectangle(&image, 45, 85, 10, 10, ColorAlpha(WHITE, tonemap(2.0f)));
        ExportImage(image, "initial.smoke.png");
    }

    if (1)
    {
        Image image = GenImageColor(100, 100, ColorAlpha(BLACK, 0.0f));
        ImageDrawRectangle(&image, 15, 55, 70, 10, ColorAlpha(WHITE, tonemap(2.0f/10.0f)));
        ExportImage(image, "target.smoke.png");
    }
}
