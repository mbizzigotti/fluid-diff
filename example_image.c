#include "raylib.h"

int main(void)
{
    InitWindow(800, 600, "Texture Example");

    // Create a 256x256 image filled with black
    Image image = GenImageColor(256, 256, BLACK);

    // Draw a simple gradient into the image
    for (int y = 0; y < image.height; y++) {
        for (int x = 0; x < image.width; x++) {
            Color c = {
                (unsigned char)x,
                (unsigned char)y,
                128,
                255
            };

            ImageDrawPixel(&image, x, y, c);
        }
    }

    // Save image to disk
    ExportImage(image, "output.png");

    // Upload image to GPU as a texture
    Texture2D texture = LoadTextureFromImage(image);

    // CPU image no longer needed
    UnloadImage(image);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawTexture(texture, 100, 100, WHITE);

        DrawText("Texture generated in memory", 100, 70, 20, BLACK);

        EndDrawing();
    }

    UnloadTexture(texture);
    CloseWindow();

    return 0;
}
