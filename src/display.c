#include "../include/raylib.h"
#include <stdio.h>
#include "../include/display.h"
#include "../include/settings.h"

void display_run(f32* img, int width, int height, volatile int* render_done, sz* samples_done) {
	SetConfigFlags(FLAG_WINDOW_HIGHDPI);
	SetTraceLogLevel(LOG_NONE); // shut up bro
    InitWindow(width, height, "render preview");
    SetTargetFPS(30);

    Image im = {
        .data = img,
        .width = width,
        .height = height,
        .mipmaps = 1,
        // .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
        .format = PIXELFORMAT_UNCOMPRESSED_R32G32B32
    };
    Texture2D tex = LoadTextureFromImage(im);

    while (!WindowShouldClose()) {
        UpdateTexture(tex, img);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(tex, 0, 0, WHITE);
        char s[100];
        sprintf(s, "samples: %zu/%d", *samples_done, N_SAMPLES);
        DrawText(*render_done ? "done" : s, 10, 10, 20, WHITE);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
}
