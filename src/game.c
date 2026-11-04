#include "game.h"
#include "physics.h"
#include "projection.h"
#include "render.h"

static void InitPlayer(Player *p) {
    p->x = 180.0f;
    p->y = 40.0f;
    p->z = PLAYER_BASE_Z;
    p->speed = PLAYER_BASE_SPEED;

    p->levitationTimer = 0.0f;
    p->floatingOffset = 0.0f;

    p->bounceForceX = 0.0f;
    p->bounceForceY = 0.0f;
    p->bounceLockoutTimer = 0.0f;
}

static void InitWorldMap(Game *game) {
    game->roomCount = 3;
    game->currentRoom = 0;

    // Room 0: Caverns of Doom
    game->rooms[0] = (Room){
        .width = 360.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = RED,
        .name = "CAVERNS OF DOOM",
        .doors = {
            { 10, WALL_BACK,  180.0f, 60.0f, 85.0f, RED,    2, 30, DOOR_CLOSED, 0.0f },
            { 11, WALL_RIGHT,  60.0f, 35.0f, 85.0f, MAROON, 1, 20, DOOR_OPEN, 1.0f },
            { 12, WALL_RIGHT, 160.0f, 35.0f, 85.0f, MAROON, 1, 21, DOOR_FREE_EXIT, 1.0f } 
        },
        .doorCount = 3
    };

    // Room 1: Gatehouse Level
    game->rooms[1] = (Room){
        .width = 240.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = GREEN,
        .name = "GATEHOUSE LEVEL",
        .doors = {
            { 20, WALL_LEFT,   60.0f, 35.0f, 85.0f, MAROON, 0, 11, DOOR_OPEN, 1.0f },
            { 21, WALL_LEFT,  160.0f, 35.0f, 85.0f, MAROON, 0, 12, DOOR_OPEN, 1.0f  },
            { 22, WALL_RIGHT, 110.0f, 35.0f, 85.0f, RED,    2, 31, DOOR_OPEN, 1.0f }
        },
        .doorCount = 3
    };

    // Room 2: The Crossroads
    game->rooms[2] = (Room){
        .width = 560.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = BLUE,
        .name = "THE CROSSROADS",
        .doors = {
            { 30, WALL_BACK, 280.0f, 60.0f, 85.0f, RED, 0, 10, DOOR_OPEN, 1.0f },
            { 31, WALL_BACK,  80.0f, 60.0f, 85.0f, RED, 1, 22, DOOR_OPEN, 1.0f }
        },
        .doorCount = 2
    };
}

void InitGame(Game *game) {
    InitPlayer(&game->player);
    InitWorldMap(game);
    game->cameraScroll = (Vector2){ 0.0f, 0.0f };
    game->projectedPlayer = (Vector2){ 0.0f, 0.0f };
}

static void UpdateGameCamera(Game *game) {
    Room *r = &game->rooms[game->currentRoom];
    game->projectedPlayer = ProjectRoomPoint(game->player.x, game->player.y, r->width);

    float screenCenterX = SCREEN_WIDTH / 2.0f;
    float screenCenterY = 260.0f;

    game->cameraScroll.x = screenCenterX - game->projectedPlayer.x;
    game->cameraScroll.y = screenCenterY - game->projectedPlayer.y;
}

void UpdateGame(Game *game) {
    float dt = GetFrameTime();

    float prevX = game->player.x;
    float prevY = game->player.y;

    float moveX = 0.0f;
    float moveY = 0.0f;

    UpdateDoorAnimations(&game->rooms[game->currentRoom], dt);
    UpdatePlayerPhysics(game, &moveX, &moveY, dt);
    UpdateCollisions(game, prevX, prevY, moveX, moveY);
    UpdateGameCamera(game);
}

void RenderGame(const Game *game) {
    const Room *room = &game->rooms[game->currentRoom];

    BeginDrawing();
    ClearBackground(BLACK);

    DrawFloorGrid(room, game->cameraScroll);
    DrawWalls(room, game->cameraScroll);
    DrawDoors(room, game->cameraScroll);
    DrawPlayer(game);
    DrawHUD(game);

    EndDrawing();
}
