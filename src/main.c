#include "raylib.h"
#include "config.h"
#include "game.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);

    Game game;
    InitGame(&game);

    while (!WindowShouldClose()) {
        UpdateGame(&game);
        RenderGame(&game);
    }

    CloseWindow();
    return 0;
}
