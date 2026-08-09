#ifndef _DISPLAY_C
#define _DISPLAY_C

#include "raylib.h"
#include "types.h"

// call this from main() — raylib must run on the main thread on macOS
static void display_run(u8* img, int width, int height, volatile int* render_done) {
    InitWindow(width, height, "render preview");
    SetTargetFPS(15);

    Image im = {
        .data = img,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8   // matches your img layout exactly
    };
    Texture2D tex = LoadTextureFromImage(im);

    while (!WindowShouldClose()) {
        // straight re-upload of whatever img currently holds — no locking, may tear
        UpdateTexture(tex, img);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(tex, 0, 0, WHITE);
        DrawText(*render_done ? "done" : "rendering...", 10, 10, 20, WHITE);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
}

#endif
