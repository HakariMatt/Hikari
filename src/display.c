#include "../include/raylib.h"
#include <math.h>
#include <stdio.h>
#include "../include/display.h"
#include "../include/settings.h"
#include "../include/log.h"

#define PREVIEW_MAX_WIDTH 960
#define PREVIEW_MAX_HEIGHT 720

void display_run(render_args* args) {
	SetConfigFlags(FLAG_WINDOW_HIGHDPI);
	SetTraceLogLevel(LOG_NONE); // shut up bro
    InitWindow(PREVIEW_MAX_WIDTH, PREVIEW_MAX_HEIGHT, "render preview");
    SetTargetFPS(30);

    f32 tex_scale = fmin(PREVIEW_MAX_WIDTH / (f32)args->width, PREVIEW_MAX_HEIGHT / (f32)args->height);

    Image im = {
        .data = args->img,
        .width = args->width,
        .height = args->height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R32G32B32
    };
    Texture2D tex = LoadTextureFromImage(im);

	while (!WindowShouldClose()) {
		UpdateTexture(tex, args->img);

		char s[100];
		if (IsKeyPressed(KEY_BACKSPACE)) args->state->should_stop = 1;

		if (args->state->should_stop) {
			sprintf(s, "Cancelling...");
		} else {
			sprintf(s, "sampling... %zu/%d", args->state->samples_done, N_SAMPLES);
		}

		BeginDrawing();
		ClearBackground(BLACK);
		DrawTextureEx(tex, (Vector2){(PREVIEW_MAX_WIDTH-(args->width*tex_scale))/2.0, (PREVIEW_MAX_HEIGHT-(args->height*tex_scale))/2.0}, 0, tex_scale, WHITE);
		DrawText(args->state->done ? "done" : s, 10, 10, 20, WHITE);
		EndDrawing();
	}
	if (!args->state->done) {
		args->state->should_stop = 1;
		print(INFO, "Cancelling render at %zu/%zu samples", args->state->samples_done, N_SAMPLES);
	}

    UnloadTexture(tex);
    CloseWindow();
}
