#ifndef _DISPLAY_C
#define _DISPLAY_C

#include "raylib.h"
#include "types.h"

// call this from main() — raylib must run on the main thread on macOS
static void display_run(u8* img, int width, int height, volatile int* render_done) {
	SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(width, height, "render preview");
    SetTargetFPS(30);

    Image im = {
        .data = img,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
    };
    Texture2D tex = LoadTextureFromImage(im);

    while (!WindowShouldClose()) {
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
