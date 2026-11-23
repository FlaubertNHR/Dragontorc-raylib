#include "game.h"
#include "map_data.h"
#include "math.h"
#include "physics.h"
#include "projection.h"
#include "render.h"
#include <stdio.h>

#if DEBUG_POSITIONS
static void DebugPrintPositions(const Game *game, float dt) {
  static float debugTimer = 0.0f;
  debugTimer += dt;

  // Print twice per second (every 0.5s) to prevent terminal flooding
  if (debugTimer >= 0.5f) {
    debugTimer = 0.0f;

    const Player *p = &game->player;
    const Servant *s = &game->servant;

    // Screen coordinates for Maroc's base/shadow
    float marocScreenX = game->projectedPlayer.x + game->cameraScroll.x;
    float marocScreenY = game->projectedPlayer.y + game->cameraScroll.y;

    printf("\n==================== DEBUG POSITIONS ====================\n");
    printf("[MAROC]   Logical: (X: %7.2f, Y: %7.2f, Alt: %5.2f) | Screen: (X: %6.1f, Y: %6.1f)\n",
           p->x, p->y, p->z + p->floatingOffset, marocScreenX, marocScreenY);

    if (s->active) {
      printf("[SERVANT] Logical: (X: %7.2f, Y: %7.2f)            | Screen: (X: %6.1f, Y: %6.1f)\n",
             s->x, s->y, s->screenX, s->screenY);
    } else {
      printf("[SERVANT] Status: INACTIVE\n");
    }
    printf("========================================================\n");
  }
}
#endif

static void InitPlayer(Player *p) {
  p->x = INITIAL_PLAYER_X;
  p->y = INITIAL_PLAYER_Y;
  p->z = 0.0f;
  p->speed = PLAYER_BASE_SPEED;

  p->levitationTimer = 0.0f;
  p->floatingOffset = 0.0f;
  p->moveRamp = 0.0f;

  p->lastDirX = 0.0f;
  p->lastDirY = 0.0f;

  p->bounceForceX = 0.0f;
  p->bounceForceY = 0.0f;
  p->bounceLockoutTimer = 0.0f;

  p->inventoryCount = 0;
  p->pickupLockoutItemId = 0;
}

void InitGame(Game *game) {
  InitPlayer(&game->player);
  game->roomCount = LoadMapData(game->rooms, MAX_ROOMS);
  game->currentRoom = 0;
  game->cameraScroll = (Vector2){0.0f, 0.0f};
  game->projectedPlayer = (Vector2){0.0f, 0.0f};

  // Texture loading
  game->frameTexture = LoadTexture("resources/Frame.png");

  // Initialize servant entity & control mode
  game->controlMode = CONTROL_MAROC;
  game->servant.active = false;
  game->servant.speed = SERVANT_BASE_SPEED;
  game->servant.carriedItemId = 0;
}

void CloseGame(Game *game) {
  if (game->frameTexture.id > 0) UnloadTexture(game->frameTexture); // Free texture
}

static void UpdateGameCamera(Game *game) {
  Room *r = &game->rooms[game->currentRoom];
  game->projectedPlayer =
      ProjectRoomPoint(game->player.x, game->player.y, r->width);

  // Viewport
  float viewportCenterX = VIEWPORT_X + (VIEWPORT_WIDTH / 2.0f);
  float viewportCenterY = VIEWPORT_Y + (VIEWPORT_HEIGHT * CAMERA_VIEWPORT_CENTER_Y_RATIO);

  game->cameraScroll.x = viewportCenterX - game->projectedPlayer.x;
  game->cameraScroll.y = viewportCenterY - game->projectedPlayer.y;
}

void UpdateGame(Game *game) {
  float dt = GetFrameTime();

  // Toggle Servant with KEY_S
  if (IsKeyPressed(KEY_S)) {
    if (game->controlMode == CONTROL_MAROC) {
      game->controlMode = CONTROL_SERVANT;
      game->servant.active = true;
      game->servant.x = game->player.x;
      game->servant.y = game->player.y;
      game->servant.speed = SERVANT_BASE_SPEED;

      // Compute exact screen position of Maroc's head
      // game->projectedPlayer + game->cameraScroll is the center of Maroc's
      // floor shadow
      float shadowScreenX = game->projectedPlayer.x + game->cameraScroll.x;
      float shadowScreenY = game->projectedPlayer.y + game->cameraScroll.y;

      float currentAltitude = game->player.z + game->player.floatingOffset;

      game->servant.screenX = shadowScreenX;
      game->servant.screenY =
          shadowScreenY - PLAYER_HEIGHT - currentAltitude - SERVANT_HEAD_OFFSET;
    } else {
      if (game->servant.carriedItemId != 0) {
        StartServantItemDrop(game);
      }
      game->controlMode = CONTROL_MAROC;
      game->servant.active = false;
    }
  }

  UpdateDoorAnimations(&game->rooms[game->currentRoom], dt);
  UpdateFallingItems(game, dt);
  if (game->controlMode == CONTROL_MAROC) {
    float prevX = game->player.x;
    float prevY = game->player.y;
    float moveX = 0.0f;
    float moveY = 0.0f;

    UpdatePlayerPhysics(game, &moveX, &moveY, dt);
    UpdateCollisions(game, prevX, prevY, moveX, moveY);
    UpdateItemPickups(game); // <--- RECOGIDA DE OBJETOS CON LA SOMBRA DE MAROC
  } else {
    // Maroc in meditative trance
    game->player.levitationTimer += dt * LEVITATION_SPEED;
    game->player.floatingOffset = sinf(game->player.levitationTimer) * LEVITATION_AMPLITUDE;

    // Servant takes exclusive movement
    UpdateServantPhysics(game, dt);
    UpdateServantDoorInteraction(game);
    UpdateServantItemInteraction(game, dt);
  }
#if DEBUG_POSITIONS
  DebugPrintPositions(game, dt);
#endif
  UpdateGameCamera(game);
}

void RenderGame(const Game *game) {
  const Room *room = &game->rooms[game->currentRoom];
  const Player *player = &game->player;

  BeginDrawing();
  ClearBackground(BLACK);

  //  RENDERING OF THE PLAYABLE AREA WITH CLIPPING (Scissor Mode)
  BeginScissorMode((int)VIEWPORT_X, (int)VIEWPORT_Y, (int)VIEWPORT_WIDTH,
                   (int)VIEWPORT_HEIGHT);
  {
    DrawFloorGrid(room, game->cameraScroll);
    DrawWalls(room, game->cameraScroll);
    DrawDoorsBehind(room, player, game->cameraScroll);

    DrawRoomItems(room, game->cameraScroll);
    DrawFallingItems(game);

    DrawPlayer(game);
    DrawDoorsInFront(room, player, game->cameraScroll);

    DrawServant(&game->servant, room->width, game->cameraScroll, game);
  }
  EndScissorMode();

  DrawHUD(game);
  DrawDecorativeFrame(game);
  EndDrawing();
}
