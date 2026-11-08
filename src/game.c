#include "game.h"
#include "map_data.h"
#include "math.h"
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

void InitGame(Game *game) {
  InitPlayer(&game->player);
  game->roomCount = LoadMapData(game->rooms, MAX_ROOMS);
  game->currentRoom = 0;
  game->cameraScroll = (Vector2){0.0f, 0.0f};
  game->projectedPlayer = (Vector2){0.0f, 0.0f};

  // Initialize servant entity & control mode
  game->controlMode = CONTROL_MAROC;
  game->servant.active = false;
  game->servant.speed = 260.0f;
  game->servant.carriedItemId = 0;
}

static void UpdateGameCamera(Game *game) {
  Room *r = &game->rooms[game->currentRoom];
  game->projectedPlayer =
      ProjectRoomPoint(game->player.x, game->player.y, r->width);

  float screenCenterX = SCREEN_WIDTH / 2.0f;
  float screenCenterY = 260.0f;

  game->cameraScroll.x = screenCenterX - game->projectedPlayer.x;
  game->cameraScroll.y = screenCenterY - game->projectedPlayer.y;
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
      game->servant.speed = 260.0f;

      // Compute exact screen position of Maroc's head
      // game->projectedPlayer + game->cameraScroll is the center of Maroc's
      // floor shadow
      float shadowScreenX = game->projectedPlayer.x + game->cameraScroll.x;
      float shadowScreenY = game->projectedPlayer.y + game->cameraScroll.y;

      float currentAltitude = game->player.z + game->player.floatingOffset;
      const float headOffset = 30.0f; // Floating above head

      game->servant.screenX = shadowScreenX;
      game->servant.screenY =
          shadowScreenY - PLAYER_HEIGHT - currentAltitude - headOffset;
    } else {
      game->controlMode = CONTROL_MAROC;
      game->servant.active = false;
    }
  }

  UpdateDoorAnimations(&game->rooms[game->currentRoom], dt);
  if (game->controlMode == CONTROL_MAROC) {
    float prevX = game->player.x;
    float prevY = game->player.y;
    float moveX = 0.0f;
    float moveY = 0.0f;

    UpdatePlayerPhysics(game, &moveX, &moveY, dt);
    UpdateCollisions(game, prevX, prevY, moveX, moveY);
  } else {
    // Maroc in meditative trance
    game->player.levitationTimer += dt * 4.0f;
    game->player.floatingOffset = sinf(game->player.levitationTimer) * 3.5f;

    // Servant takes exclusive movement
    UpdateServantPhysics(game, dt);
    UpdateServantDoorInteraction(game); // Chequeo de puertas del sirvient
  }

  UpdateGameCamera(game);
}

void RenderGame(const Game *game) {
  const Room *room = &game->rooms[game->currentRoom];
  const Player *player = &game->player;

  BeginDrawing();
  ClearBackground(BLACK);

  DrawFloorGrid(room, game->cameraScroll);
  DrawWalls(room, game->cameraScroll);
  DrawDoorsBehind(room, player, game->cameraScroll);
  DrawPlayer(game);
  DrawDoorsInFront(room, player, game->cameraScroll);

  DrawServant(&game->servant, room->width, game->cameraScroll, game);

  DrawHUD(game);

  EndDrawing();
}
