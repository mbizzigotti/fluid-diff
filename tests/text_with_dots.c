// text_with_dots.c -- Generated via Claude Opus 4.8 (Mostly)
//
// Draws a line of text onto an image and places a small filled circle
// centered underneath each visible character, then exports the image.
// Text, font, font size, dot radius, dot gap, spacing and padding are all
// configurable at runtime; the image size is computed from the laid-out
// text plus the padding values.
//
// Build (Linux):
//   gcc text_with_dots.c -o text_with_dots -lraylib -lm -lpthread -ldl -lrt -lX11
// Build (macOS):
//   clang text_with_dots.c -o text_with_dots -lraylib \
//         -framework Cocoa -framework IOKit -framework OpenGL
// Build (Windows, MinGW):
//   gcc text_with_dots.c -o text_with_dots.exe -lraylib -lopengl32 -lgdi32 -lwinmm
//
// Examples:
//   ./text_with_dots -t "Hello, raylib!" -s 48 -r 5 -g 20
//   ./text_with_dots -t "Custom font" -f /path/to/Font.ttf -s 64 --pad 40 -o out.png

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static inline float tonemap(float x)
{
    return x / (x + 1);
}

static void usage(const char *prog)
{
    printf(
        "Usage: %s [options]\n"
        "  -t, --text <str>     Text to render        (default: \"Hello, raylib!\")\n"
        "  -f, --font <path>    TTF font file         (default: raylib's built-in font)\n"
        "  -s, --size <px>      Font size in pixels   (default: 40)\n"
        "  -r, --radius <px>    Dot radius in pixels  (default: 4)\n"
        "  -g, --gap <px>       Gap text->dot center  (default: 16)\n"
        "      --spacing <px>   Extra char spacing    (default: 4)\n"
        "  -p, --pad <px>       Padding on all sides  (default: 24)\n"
        "      --pad-x <px>     Horizontal padding    (overrides --pad)\n"
        "      --pad-y <px>     Vertical padding      (overrides --pad)\n"
        "  -o, --output <path>  Output PNG file       (default: text_with_dots.png)\n"
        "  -h, --help           Show this help\n",
        prog);
}

#define NEED_VAL(i, argc) \
    do { if ((i) + 1 >= (argc)) { fprintf(stderr, "Missing value for %s\n", argv[i]); usage(argv[0]); return 1; } } while (0)

// Horizontal advance raylib uses to step from one glyph to the next.
static float GlyphAdvance(Font font, int idx, float scale)
{
    return (font.glyphs[idx].advanceX == 0)
         ? font.recs[idx].width * scale
         : (float)font.glyphs[idx].advanceX * scale;
}

int main(int argc, char **argv)
{
    // ---- Runtime-configurable defaults ------------------------------------
    const char *text     = "Hello, raylib!";
    const char *fontPath = NULL;                 // NULL -> built-in default font
    const char *outPath  = "text_with_dots.png";
    float fontSize  = 40.0f;
    int   dotRadius = 4;
    float dotGap    = 16.0f;
    float spacing   = 4.0f;
    float padX      = 24.0f;
    float padY      = 24.0f;
    bool render_dots = false;
    const Color textColor = ColorAlpha(WHITE, tonemap(2.0f/8.0f));
    const Color dotColor  = ColorAlpha(WHITE, tonemap(8.0f));
    const Color bgColor   = {};

    // ---- Parse command-line arguments -------------------------------------
    for (int i = 1; i < argc; i++)
    {
        const char *a = argv[i];
        if      (!strcmp(a, "-t") || !strcmp(a, "--text"))    { NEED_VAL(i, argc); text     = argv[++i]; }
        else if (!strcmp(a, "-f") || !strcmp(a, "--font"))    { NEED_VAL(i, argc); fontPath = argv[++i]; }
        else if (!strcmp(a, "-o") || !strcmp(a, "--output"))  { NEED_VAL(i, argc); outPath  = argv[++i]; }
        else if (!strcmp(a, "-s") || !strcmp(a, "--size"))    { NEED_VAL(i, argc); fontSize  = (float)atof(argv[++i]); }
        else if (!strcmp(a, "-r") || !strcmp(a, "--radius"))  { NEED_VAL(i, argc); dotRadius = atoi(argv[++i]); }
        else if (!strcmp(a, "-g") || !strcmp(a, "--gap"))     { NEED_VAL(i, argc); dotGap    = (float)atof(argv[++i]); }
        else if (!strcmp(a, "--spacing"))                     { NEED_VAL(i, argc); spacing   = (float)atof(argv[++i]); }
        else if (!strcmp(a, "-p") || !strcmp(a, "--pad"))     { NEED_VAL(i, argc); padX = padY = (float)atof(argv[++i]); }
        else if (!strcmp(a, "--pad-x"))                       { NEED_VAL(i, argc); padX = (float)atof(argv[++i]); }
        else if (!strcmp(a, "--pad-y"))                       { NEED_VAL(i, argc); padY = (float)atof(argv[++i]); }
        else if (!strcmp(a, "-k"))                            { render_dots = true; }
        else if (!strcmp(a, "-h") || !strcmp(a, "--help"))    { usage(argv[0]); return 0; }
        else { fprintf(stderr, "Unknown option: %s\n", a); usage(argv[0]); return 1; }
    }

    // ---- raylib setup -----------------------------------------------------
    // The built-in font and the GL context both come from InitWindow(); the
    // window stays hidden since we only export an image. The window size is
    // irrelevant because we draw to an Image, not to the framebuffer.
    SetTraceLogLevel(LOG_WARNING);          // quiet the startup chatter
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "text with dots");

    bool customFont = false;
    Font font = GetFontDefault();
    if (fontPath)
    {
        Font loaded = LoadFontEx(fontPath, (int)(fontSize + 0.5f), NULL, 0);
        // raylib returns the default font on failure; detect a real load.
        if (loaded.glyphCount > 0 && loaded.texture.id != GetFontDefault().texture.id)
        {
            font = loaded;
            customFont = true;
        }
        else TraceLog(LOG_WARNING, "Could not load font '%s'; using default font", fontPath);
    }
    float scale = fontSize / (float)font.baseSize;

    // ---- Pass 1: lay out glyphs to find the content width -----------------
    // We position glyphs ourselves (rather than reverse-engineering raylib's
    // base-size-then-upscale layout inside ImageDrawTextEx) so the dots and
    // the characters share the exact same cursor and stay aligned.
    float cursor = 0.0f;       // pen x, content-relative
    float contentRight = 0.0f; // rightmost inked pixel
    for (int i = 0; text[i] != '\0'; i++)
    {
        int idx = GetGlyphIndex(font, (unsigned char)text[i]);
        float gl = cursor + font.glyphs[idx].offsetX * scale;
        float gr = gl + font.recs[idx].width * scale;
        if (gr > contentRight) contentRight = gr;
        cursor += GlyphAdvance(font, idx, scale) + spacing;
    }
    if (contentRight < 0.0f) contentRight = 0.0f;

    int imgW = (int)(contentRight + 2.0f * padX + 0.5f);
    int imgH = (int)(fontSize + dotGap + (float)dotRadius + 2.0f * padY + 0.5f);
    if (imgW < 1) imgW = 1;
    if (imgH < 1) imgH = 1;

    // ---- Pass 2: draw each glyph and the dot beneath it -------------------
    Image canvas = GenImageColor(imgW, imgH, bgColor);

    float penX = padX;
    int   dotY = (int)(padY + fontSize + dotGap + 0.5f);   // below the text

    for (int i = 0; text[i] != '\0'; i++)        // ASCII; see note for UTF-8
    {
        unsigned char c   = (unsigned char)text[i];
        int           idx = GetGlyphIndex(font, c);
        char          ch[2] = { (char)c, '\0' };

        // Draw just this glyph at the cursor. ImageDrawTextEx applies the
        // glyph's own offsetX/offsetY internally, so baselines still line up.
        if (!render_dots)
            ImageDrawTextEx(&canvas, font, ch, (Vector2){ penX, padY }, fontSize, 0.0f, textColor);

        // Dot centered under the glyph's visible box, from the same cursor.
        float glyphCenter = penX + font.glyphs[idx].offsetX * scale
                                 + (font.recs[idx].width * scale) * 0.5f;
        if (render_dots && c != ' ' && c != '\t')
            ImageDrawCircle(&canvas, (int)(glyphCenter + 0.5f), dotY, dotRadius, dotColor);

        penX += GlyphAdvance(font, idx, scale) + spacing;
    }

    // ---- Save -------------------------------------------------------------
    bool ok = ExportImage(canvas, outPath);
    if (ok) printf("Saved %dx%d image to %s\n", imgW, imgH, outPath);
    else    fprintf(stderr, "Failed to export image to %s\n", outPath);

    // ---- Cleanup ----------------------------------------------------------
    UnloadImage(canvas);
    if (customFont) UnloadFont(font);
    CloseWindow();
    return ok ? 0 : 1;
}