#include "../include/raylib.h"
#include "../include/display.h"

void display_run(u8* img, int width, int height, volatile int* render_done) {
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
