#include "render.h"
#include "projection.h"
#include <math.h>
#include <stddef.h>

// Draws the outer ornamental frame (sides and top section)
void DrawDecorativeFrame(const Game *game) {
  // RENDERING THE PNG FRAME TEXTURE, SCALED TO THE GAME'S RESOLUTION
  if (game != NULL && game->frameTexture.id > 0) {
    // Origin coordinates within the original image (1024x768)
    Rectangle sourceRect = {0.0f, 0.0f, (float)game->frameTexture.width,
                            (float)game->frameTexture.height};

    // Destination rectangle on screen (adapted to SCREEN_WIDTH and SCREEN_HEIGHT)
    Rectangle destRect = {0.0f, 0.0f, (float)SCREEN_WIDTH,
                          (float)SCREEN_HEIGHT};
    Vector2 origin = {0.0f, 0.0f};

    DrawTexturePro(game->frameTexture, sourceRect, destRect, origin, 0.0f,
                   WHITE);
    return;
  }
  Color outerFrameColor = (Color){35, 30, 45, 255}; // Dark stone tone
  Color bevelLightColor = (Color){70, 65, 85, 255}; // Glossy bevel
  Color bevelDarkColor = (Color){15, 12, 20, 255};  // Shadow bevel

  // 1. Top strip
  DrawRectangle(0, 0, SCREEN_WIDTH, (int)FRAME_THICKNESS, outerFrameColor);
  // 2. Left strip
  DrawRectangle(0, (int)FRAME_THICKNESS, (int)FRAME_THICKNESS,
                SCREEN_HEIGHT - (int)FRAME_THICKNESS, outerFrameColor);
  // 3. Right strip
  DrawRectangle(SCREEN_WIDTH - (int)FRAME_THICKNESS, (int)FRAME_THICKNESS,
                (int)FRAME_THICKNESS, SCREEN_HEIGHT - (int)FRAME_THICKNESS,
                outerFrameColor);

  // --- RETRO DECORATIVE BEvELS AROUND THE VIEWPORT ---
  DrawRectangleLinesEx((Rectangle){VIEWPORT_X - 2, VIEWPORT_Y - 2,
                                   VIEWPORT_WIDTH + 4, VIEWPORT_HEIGHT + 4},
                       2.0f, bevelDarkColor);
  DrawRectangleLinesEx((Rectangle){VIEWPORT_X - 4, VIEWPORT_Y - 4,
                                   VIEWPORT_WIDTH + 8, VIEWPORT_HEIGHT + 8},
                       2.0f, bevelLightColor);
}

void DrawFloorGrid(const Room *room, Vector2 scroll) {
  float leftFrontX = GetLeftWallX(0.0f, room->width, room->length);
  float leftBackX = GetLeftWallX(room->length, room->width, room->length);
  float rightFrontX = GetRightWallX(0.0f, room->width, room->length);
  float rightBackX = GetRightWallX(room->length, room->width, room->length);

  for (float x = 0; x <= room->width; x += TILE_SIZE) {
    float t = (room->width > 0.0f) ? (x / room->width) : 0.0f;
    float xFront = leftFrontX + t * (rightFrontX - leftFrontX);
    float xBack = leftBackX + t * (rightBackX - leftBackX);

    Vector2 p1 = ProjectRoomPoint(xFront, 0.0f, room->width);
    Vector2 p2 = ProjectRoomPoint(xBack, room->length, room->width);
    DrawLine(p1.x + scroll.x, p1.y + scroll.y, p2.x + scroll.x, p2.y + scroll.y,
             DARKGRAY);
  }

  for (float y = 0; y <= room->length; y += TILE_SIZE) {
    float xLeft = GetLeftWallX(y, room->width, room->length);
    float xRight = GetRightWallX(y, room->width, room->length);
    Vector2 p1 = ProjectRoomPoint(xLeft, y, room->width);
    Vector2 p2 = ProjectRoomPoint(xRight, y, room->width);
    DrawLine(p1.x + scroll.x, p1.y + scroll.y, p2.x + scroll.x, p2.y + scroll.y,
             DARKGRAY);
  }
}

void DrawWalls(const Room *room, Vector2 scroll) {
  float leftFrontX = GetLeftWallX(0.0f, room->width, room->length);
  float leftBackX = GetLeftWallX(room->length, room->width, room->length);
  float rightFrontX = GetRightWallX(0.0f, room->width, room->length);
  float rightBackX = GetRightWallX(room->length, room->width, room->length);

  Vector2 leftBottom = ProjectRoomPoint(leftFrontX, 0.0f, room->width);
  Vector2 leftTop = ProjectRoomPoint(leftBackX, room->length, room->width);
  Vector2 rightTop = ProjectRoomPoint(rightBackX, room->length, room->width);
  Vector2 rightBottom = ProjectRoomPoint(rightFrontX, 0.0f, room->width);

  leftBottom.x += scroll.x;
  leftBottom.y += scroll.y;
  leftTop.x += scroll.x;
  leftTop.y += scroll.y;
  rightTop.x += scroll.x;
  rightTop.y += scroll.y;
  rightBottom.x += scroll.x;
  rightBottom.y += scroll.y;

  // Left wall
  DrawLineV(leftBottom, leftTop, room->wallColor);
  DrawLineV((Vector2){leftBottom.x, leftBottom.y - room->wallHeight},
            (Vector2){leftTop.x, leftTop.y - room->wallHeight},
            room->wallColor);
  DrawLineV(leftBottom,
            (Vector2){leftBottom.x, leftBottom.y - room->wallHeight},
            room->wallColor);
  DrawLineV(leftTop, (Vector2){leftTop.x, leftTop.y - room->wallHeight},
            room->wallColor);

  // Back wall
  DrawLineV(leftTop, rightTop, room->wallColor);
  DrawLineV((Vector2){leftTop.x, leftTop.y - room->wallHeight},
            (Vector2){rightTop.x, rightTop.y - room->wallHeight},
            room->wallColor);
  DrawLineV(rightTop, (Vector2){rightTop.x, rightTop.y - room->wallHeight},
            room->wallColor);

  // Right wall
  DrawLineV(rightTop, rightBottom, room->wallColor);
  DrawLineV((Vector2){rightBottom.x, rightBottom.y - room->wallHeight},
            (Vector2){rightTop.x, rightTop.y - room->wallHeight},
            room->wallColor);
  DrawLineV(rightBottom,
            (Vector2){rightBottom.x, rightBottom.y - room->wallHeight},
            room->wallColor);
}

static void DrawBackDoorHole(const Door *door, const Room *room,
                             Vector2 scroll) {
  Vector2 doorBase =
      ProjectRoomPoint(door->position, room->length, room->width);
  doorBase.x += scroll.x;
  doorBase.y += scroll.y;

  Color fillColor = (door->state == DOOR_CLOSED && door->openProgress <= 0.0f)
                        ? DARKGRAY
                        : BLACK;
  DrawRectangle(doorBase.x - door->width / 2.0f, doorBase.y - door->height,
                door->width, door->height, fillColor);
  DrawRectangleLinesEx((Rectangle){doorBase.x - door->width / 2.0f,
                                   doorBase.y - door->height, door->width,
                                   door->height},
                       2.0f, door->color);

  if (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) {
    DrawText("X", doorBase.x - 5, doorBase.y - door->height / 2.0f - 5, 12,
             RED);
    Vector2 knobPos = {doorBase.x + (door->width * 0.25f),
                       doorBase.y - (door->height * 0.45f)};
    DrawCircleV(knobPos, 3.0f, GOLD);
    DrawCircleLines((int)knobPos.x, (int)knobPos.y, 3.5f, DARKBROWN);
  }
}

static void DrawBackDoorLeaf(const Door *door, const Room *room,
                             Vector2 scroll) {
  if (door->state == DOOR_FREE_EXIT || door->openProgress <= 0.0f)
    return;

  Vector2 doorBase =
      ProjectRoomPoint(door->position, room->length, room->width);
  doorBase.x += scroll.x;
  doorBase.y += scroll.y;

  Vector2 hingeBottom = {doorBase.x - door->width / 2.0f, doorBase.y};
  Vector2 hingeTop = {hingeBottom.x, hingeBottom.y - door->height};

  float leafLength = door->width * 0.90f;
  const float MAX_ANGLE = (PI / 2.0f) * 0.88f;
  float angle = door->openProgress * MAX_ANGLE;

  float dx = leafLength * cosf(angle) - (leafLength * 0.55f) * sinf(angle);
  float dy = (leafLength * 0.38f) * sinf(angle);

  Vector2 edgeBottom = {hingeBottom.x + dx, hingeBottom.y + dy};
  Vector2 edgeTop = {edgeBottom.x, edgeBottom.y - door->height};

  // Fill open back door leaf with solid door theme color
  Color doorLeafColor = door->color;

  DrawTriangle(hingeBottom, hingeTop, edgeBottom, doorLeafColor);
  DrawTriangle(hingeTop, edgeTop, edgeBottom, doorLeafColor);

  DrawLineEx(hingeBottom, edgeBottom, 2.5f, door->color);
  DrawLineEx(edgeBottom, edgeTop, 2.5f, door->color);
  DrawLineEx(edgeTop, hingeTop, 2.5f, door->color);

  const float handleRatio = 0.85f;
  Vector2 knobPos = {
      hingeBottom.x + (edgeBottom.x - hingeBottom.x) * handleRatio,
      (hingeBottom.y + (edgeBottom.y - hingeBottom.y) * handleRatio) -
          (door->height * 0.45f)};
  DrawCircleV(knobPos, 3.0f, GOLD);
  DrawCircleLines((int)knobPos.x, (int)knobPos.y, 3.5f, DARKBROWN);
}

static void DrawSideDoor(const Door *door, const Room *room, Vector2 scroll) {
  float yFront = door->position - (door->width / 2.0f);
  float yBack = door->position + (door->width / 2.0f);
  float xWallFront = (door->wall == WALL_LEFT) ? GetLeftWallX(yFront, room->width, room->length) : GetRightWallX(yFront, room->width, room->length);
  float xWallBack = (door->wall == WALL_LEFT) ? GetLeftWallX(yBack, room->width, room->length) : GetRightWallX(yBack, room->width, room->length);

  Vector2 baseFront = ProjectRoomPoint(xWallFront, yFront, room->width);
  Vector2 baseBack = ProjectRoomPoint(xWallBack, yBack, room->width);
  baseFront.x += scroll.x;
  baseFront.y += scroll.y;
  baseBack.x += scroll.x;
  baseBack.y += scroll.y;

  Vector2 roofFront = {baseFront.x, baseFront.y - door->height};
  Vector2 roofBack = {baseBack.x, baseBack.y - door->height};

  if (door->state == DOOR_FREE_EXIT) {
    DrawTriangle(baseFront, baseBack, roofFront, BLACK);
    DrawTriangle(roofFront, baseBack, roofBack, BLACK);
    DrawLineEx(baseFront, roofFront, 2.0f, door->color);
    DrawLineEx(roofFront, roofBack, 2.0f, door->color);
    DrawLineEx(roofBack, baseBack, 2.0f, door->color);
    return;
  }

  Color fillColor = (door->state == DOOR_CLOSED && door->openProgress <= 0.0f)
                        ? DARKGRAY
                        : BLACK;

  DrawTriangle(baseFront, baseBack, roofFront, fillColor);
  DrawTriangle(roofFront, baseBack, roofBack, fillColor);

  DrawLineEx(baseFront, roofFront, 2.0f, door->color);
  DrawLineEx(roofFront, roofBack, 2.0f, door->color);
  DrawLineEx(roofBack, baseBack, 2.0f, door->color);

  if (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) {
    Vector2 doorCenter = {(baseFront.x + baseBack.x) / 2.0f,
                          (baseFront.y + baseBack.y) / 2.0f - door->height / 2.0f};
    DrawText("X", doorCenter.x - 5, doorCenter.y - 5, 12, RED);

    Vector2 knobPos = {
        baseFront.x + (baseBack.x - baseFront.x) * 0.20f,
        (baseFront.y + (baseBack.y - baseFront.y) * 0.20f) - door->height * 0.45f};
    DrawCircleV(knobPos, 3.0f, GOLD);
    DrawCircleLines((int)knobPos.x, (int)knobPos.y, 3.5f, DARKBROWN);
  } else if (door->openProgress > 0.0f) {
    Vector2 hingeBottom = baseBack;
    Vector2 hingeTop = roofBack;

    Vector2 doorwayVector = {baseFront.x - baseBack.x, baseFront.y - baseBack.y};
    float doorwayPixelSpan = sqrtf(doorwayVector.x * doorwayVector.x +
                                   doorwayVector.y * doorwayVector.y);
    float leafLength = doorwayPixelSpan * 1.1f;

    Vector2 wallDir = {0.0f, 0.0f};
    if (doorwayPixelSpan > 0.0f) {
      wallDir.x = doorwayVector.x / doorwayPixelSpan;
      wallDir.y = doorwayVector.y / doorwayPixelSpan;
    }

    float angle = door->openProgress * (PI / 2.0f);
    float inwardX = (door->wall == WALL_LEFT) ? 1.0f : -1.0f;

    Vector2 edgeBottom;
    edgeBottom.x = hingeBottom.x + (wallDir.x * leafLength * cosf(angle)) +
                   (inwardX * leafLength * sinf(angle));
    edgeBottom.y = hingeBottom.y + (wallDir.y * leafLength * cosf(angle));
    Vector2 edgeTop = {edgeBottom.x, edgeBottom.y - door->height};

    // Solid door theme color for open leaf fill
    Color doorLeafColor = door->color;
    // Correct Counter-Clockwise (CCW) vertex ordering for both side walls
    if (door->wall == WALL_LEFT) {
      DrawTriangle(hingeBottom, edgeBottom, hingeTop, doorLeafColor);
      DrawTriangle(hingeTop, edgeBottom, edgeTop, doorLeafColor);
    } else {
      DrawTriangle(hingeBottom, hingeTop, edgeBottom, doorLeafColor);
      DrawTriangle(hingeTop, edgeTop, edgeBottom, doorLeafColor);
    }

    // Door leaf outline & handle knob
    DrawLineEx(hingeBottom, edgeBottom, 2.5f, door->color);
    DrawLineEx(edgeBottom, edgeTop, 2.5f, door->color);
    DrawLineEx(edgeTop, hingeTop, 2.5f, door->color);

    const float handleRatio = 0.90f;
    float handleAltitude = door->height * 0.45f;
    Vector2 knobPos = {
        hingeBottom.x + (edgeBottom.x - hingeBottom.x) * handleRatio,
        (hingeBottom.y + (edgeBottom.y - hingeBottom.y) * handleRatio) -
            handleAltitude};
    DrawCircleV(knobPos, 3.0f, GOLD);
    DrawCircleLines((int)knobPos.x, (int)knobPos.y, 3.5f, DARKBROWN);
  }
}

// Determines if the door structure or leaf physically stands between the camera and the player
static bool IsDoorInFrontOfPlayer(const Door *door, const Room *room,
                                  const Player *player) {
  // Back wall door occlusion check
  if (door->wall == WALL_BACK) {
    float hingeX = door->position - (door->width / 2.0f);
    bool behindInY = (player->y >= (room->length - 12.0f));
    bool behindInX =
        (player->x <= (hingeX + 10.0f) && player->x >= (hingeX - door->width * 0.8f));
    return (behindInY && behindInX && door->openProgress > 0.0f);
  }

  // Side doors (WALL_LEFT & WALL_RIGHT)
  // Front edge of the doorway frame is at yFront
  float yFront = door->position - (door->width / 2.0f);

  // Horizontal span reach of the door leaf extending into the room floor
  float leafReachX = door->height * 1.2f;

  if (door->wall == WALL_LEFT) {
    bool playerDeeperThanDoor = (player->y >= yFront - 4.0f);
    float leftWallX = GetLeftWallX(player->y, room->width, room->length);
    bool playerNearLeftWall = (player->x >= (leftWallX - 10.0f) && player->x <= (leftWallX + leafReachX));
    return (playerDeeperThanDoor && playerNearLeftWall);
  } else if (door->wall == WALL_RIGHT) {
    bool playerDeeperThanDoor = (player->y >= yFront - 4.0f);
    float rightWallX = GetRightWallX(player->y, room->width, room->length);
    bool playerNearRightWall = (player->x <= (rightWallX + 10.0f) && player->x >= (rightWallX - leafReachX));
    return (playerDeeperThanDoor && playerNearRightWall);
  }

  return false;
}

// Determines if the door leaf physically stands in front of the player's sprite
static bool IsLeafInFrontOfPlayer(const Door *door, const Room *room,
                                  const Player *player) {
  if (door->wall == WALL_BACK) {
    float hingeX = door->position - (door->width / 2.0f);
    bool behindInY = (player->y >= (room->length - 12.0f));
    bool behindInX =
        (player->x <= (hingeX + 10.0f) && player->x >= (hingeX - door->width * 0.8f));
    return (behindInY && behindInX && door->openProgress > 0.0f);
  }

  // Side doors (WALL_LEFT and WALL_RIGHT): Hinge base is located at yBack
  float yBack = door->position + (door->width / 2.0f);
  float leafSpan = door->height * 0.75f;

  float leftWallX = GetLeftWallX(player->y, room->width, room->length);
  float rightWallX = GetRightWallX(player->y, room->width, room->length);

  if (door->wall == WALL_LEFT) {
    bool behindInY = (player->y > yBack);
    bool nearWallX = (player->x >= (leftWallX - 20.0f) &&
                      player->x <= (leftWallX + leafSpan + 30.0f));
    return (behindInY && nearWallX);
  } else if (door->wall == WALL_RIGHT) {
    bool behindInY = (player->y > yBack);
    bool nearWallX = (player->x <= (rightWallX + 20.0f) &&
                      player->x >= (rightWallX - leafSpan - 30.0f));
    return (behindInY && nearWallX);
  }

  return false;
}

void DrawDoorsBehind(const Room *room, const Player *player, Vector2 scroll) {
  for (int i = 0; i < room->doorCount; i++) {
    const Door *door = &room->doors[i];

    if (door->wall == WALL_BACK) {
      DrawBackDoorHole(door, room, scroll);
      if (!IsLeafInFrontOfPlayer(door, room, player)) {
        DrawBackDoorLeaf(door, room, scroll);
      }
    } else {
      // The wall frame/hole stays behind the player
      if (!IsLeafInFrontOfPlayer(door, room, player)) {
        DrawSideDoor(door, room, scroll);
      }
    }
  }
}

void DrawDoorsInFront(const Room *room, const Player *player, Vector2 scroll) {
  for (int i = 0; i < room->doorCount; i++) {
    const Door *door = &room->doors[i];

    if (IsLeafInFrontOfPlayer(door, room, player)) {
      if (door->wall == WALL_BACK) {
        DrawBackDoorLeaf(door, room, scroll);
      } else {
        // ONLY the open wooden panel is drawn over the player
        DrawSideDoor(door, room, scroll);
      }
    }
  }
}

void DrawPlayer(const Game *game) {
  Vector2 shadowPos = {
      game->projectedPlayer.x + game->cameraScroll.x,
      game->projectedPlayer.y + game->cameraScroll.y};

  // Semi-transparent floor shadow oval
  DrawEllipse(shadowPos.x, shadowPos.y, SHADOW_RADIUS_X, SHADOW_RADIUS_Y,
              (Color){255, 255, 255, 85});

  // Levitating character cube floating above the shadow
  float totalAltitude = game->player.z + game->player.floatingOffset;
  DrawRectangle(shadowPos.x - PLAYER_WIDTH / 2.0f,
                shadowPos.y - PLAYER_HEIGHT - totalAltitude, PLAYER_WIDTH,
                PLAYER_HEIGHT, WHITE);

#if DEBUG_HITBOX
  // --- VISUAL INDICATOR FOR THE EXCHANGE HITBOX (DEBUG) ---
  float bodyWidth = 20.0f;
  float bodyHeight = 14.0f;

  float marocBodyCenterX = shadowPos.x;
  float marocBodyCenterY = shadowPos.y - totalAltitude - (PLAYER_HEIGHT * 0.55f);

  Rectangle hitBox = {
      marocBodyCenterX - (bodyWidth * 0.5f),
      marocBodyCenterY - (bodyHeight * 0.5f),
      bodyWidth,
      bodyHeight
    };

  // Semi-transparent red box and yellow dot in the center of the torso
  DrawRectangleRec(hitBox, (Color){255, 0, 0, 110});
  DrawRectangleLinesEx(hitBox, 1.5f, RED);
  DrawCircleV((Vector2){marocBodyCenterX, marocBodyCenterY}, 2.0f, YELLOW);

#endif
}

// Static helper to render retro vector item icons (Key, Scroll, Crown, Treasure, Artifact)
static void DrawItemSprite(ItemCategory category, Vector2 centerPos,
                           float scale, Color color) {
  switch (category) {
  case ITEM_TYPE_KEY: { // Retro key: bow + shank + bit
    DrawCircleLines((int)centerPos.x - (int)(4.0f * scale), (int)centerPos.y,
                    4.0f * scale, color);
    DrawRectangle((int)centerPos.x, (int)centerPos.y - (int)(1.0f * scale),
                  (int)(10.0f * scale), (int)(2.0f * scale), color);
    DrawRectangle((int)centerPos.x + (int)(6.0f * scale), (int)centerPos.y,
                  (int)(2.0f * scale), (int)(4.0f * scale), color);
    DrawRectangle((int)centerPos.x + (int)(9.0f * scale), (int)centerPos.y,
                  (int)(2.0f * scale), (int)(3.0f * scale), color);
    break;
  }
  case ITEM_TYPE_SCROLL: { // Rolled-up parchment
    Rectangle scrollBody = {centerPos.x - 7.0f * scale,
                            centerPos.y - 4.0f * scale, 14.0f * scale,
                            8.0f * scale};
    DrawRectangleRec(scrollBody, RAYWHITE);
    DrawRectangleLinesEx(scrollBody, 1.5f * scale, DARKBROWN);

    DrawRectangle((int)centerPos.x - (int)(1.5f * scale), // Central red ribbon
                  (int)centerPos.y - (int)(4.0f * scale), (int)(3.0f * scale),
                  (int)(8.0f * scale), RED);
    break;
  }
  case ITEM_TYPE_CROWN: { // Royal crown with 3 points
    Vector2 p1 = {centerPos.x - 8.0f * scale, centerPos.y + 4.0f * scale};
    Vector2 p2 = {centerPos.x - 8.0f * scale, centerPos.y - 4.0f * scale};
    Vector2 p3 = {centerPos.x - 4.0f * scale, centerPos.y};
    Vector2 p4 = {centerPos.x, centerPos.y - 6.0f * scale};
    Vector2 p5 = {centerPos.x + 4.0f * scale, centerPos.y};
    Vector2 p6 = {centerPos.x + 8.0f * scale, centerPos.y - 4.0f * scale};
    Vector2 p7 = {centerPos.x + 8.0f * scale, centerPos.y + 4.0f * scale};

    DrawRectangle((int)p1.x, (int)centerPos.y, (int)(16.0f * scale),
                  (int)(4.0f * scale), GOLD);
    DrawTriangle(p1, p3, p2, GOLD);
    DrawTriangle(p3, p5, p4, GOLD);
    DrawTriangle(p5, p7, p6, GOLD);
    break;
  }
  case ITEM_TYPE_TREASURE: { // Gem / Gleaming chalice
    DrawPoly(centerPos, 4, 7.0f * scale, 45.0f, color);
    DrawPolyLines(centerPos, 4, 7.0f * scale, 45.0f, WHITE);
    break;
  }
  case ITEM_TYPE_ARTIFACT: { // Magic artifact
    DrawCircleV(centerPos, 6.0f * scale, color);
    DrawCircleLines((int)centerPos.x, (int)centerPos.y, 8.0f * scale, RAYWHITE);
    break;
  }
  }
}
void DrawRoomItems(const Room *room, Vector2 scroll) {
  for (int i = 0; i < room->itemCount; i++) {
    const Item *item = &room->items[i];
    if (!item->active)
      continue;

    Vector2 itemBaseScreen;
    if (item->isWallItem) {
      if (item->wall == WALL_BACK) {
        // Base anchor point is projected directly on the back wall floor line
        itemBaseScreen = ProjectRoomPoint(item->x, room->length, room->width);
      } else if (item->wall == WALL_LEFT) {
        float wallX = GetLeftWallX(item->y, room->width, room->length);
        itemBaseScreen = ProjectRoomPoint(wallX, item->y, room->width);
      } else { // WALL_RIGHT
        float wallX = GetRightWallX(item->y, room->width, room->length);
        itemBaseScreen = ProjectRoomPoint(wallX, item->y, room->width);
      }
    } else {
      itemBaseScreen = ProjectRoomPoint(item->x, item->y, room->width);
    }
    itemBaseScreen.x += scroll.x;
    itemBaseScreen.y += scroll.y;

    if (item->isWallItem) {
      // Elevation is measured vertically above the corresponding wall floor line
      float drawY = itemBaseScreen.y - item->wallHeightOffset;
      DrawRectangle(itemBaseScreen.x - 6, drawY - 6, 12, 12, item->color);
      DrawRectangleLines(itemBaseScreen.x - 6, drawY - 6, 12, 12, GOLD);
    } else {
      DrawCircleV(itemBaseScreen, 6.0f, item->color);
      DrawCircleLines((int)itemBaseScreen.x, (int)itemBaseScreen.y, 6.5f, WHITE);
    }
  }
}

void DrawServant(const Servant *servant, float roomWidth, Vector2 cameraScroll,
                 const Game *game) {
  (void)roomWidth;
  (void)cameraScroll;

  if (!servant->active)
    return;

  Vector2 servantCenter = {servant->screenX, servant->screenY};

  DrawCircleV(servantCenter, 8.0f, YELLOW);
  DrawCircleLines((int)servantCenter.x, (int)servantCenter.y, 8.5f, GOLD);

  DrawCircleV((Vector2){servantCenter.x - 3, servantCenter.y - 2}, 1.5f, BLACK);
  DrawCircleV((Vector2){servantCenter.x + 3, servantCenter.y - 2}, 1.5f, BLACK);

  // Render carried item floating attached right below servant (clamped on front vertical line)
  if (servant->carriedItemId != 0 && servant->carriedItem.id != 0) {
    const Room *r = &game->rooms[game->currentRoom];
    const float marginPx = 8.0f; // Margen respecto a la pared

    // 1. Computing of the front left and right exterior vertical lines
    float minScreenX = (GetLeftWallX(0.0f, r->width, r->length) * PROJECTION_SCALE) + game->cameraScroll.x + marginPx;
    float maxScreenX = (GetRightWallX(0.0f, r->width, r->length) * PROJECTION_SCALE) + game->cameraScroll.x - marginPx;

    // 2. Clamping the object's X coordinate to the outer line
    float itemScreenX = servant->screenX;
    if (itemScreenX < minScreenX) itemScreenX = minScreenX;
    if (itemScreenX > maxScreenX) itemScreenX = maxScreenX;

    Vector2 itemPos = {itemScreenX, servantCenter.y + 14.0f};
    DrawItemSprite(servant->carriedItem.category, itemPos, 0.85f, servant->carriedItem.color);
  }
}

void DrawHUD(const Game *game) {
  const Room *room = &game->rooms[game->currentRoom];

  // Base coordinates for HUD parchment (positioned behind the decorative frame)
  float hudX = FRAME_THICKNESS;
  float hudY = (SCREEN_HEIGHT - HUD_HEIGHT) + HUD_PARCHMENT_GAP;
  float hudWidth = VIEWPORT_WIDTH;
  float hudHeight = HUD_HEIGHT - HUD_PARCHMENT_GAP;

  // --- 1. PROCEDURAL PARCHMENT RENDERING (DRAGONTORC RETRO STYLE) ---
  Color parchmentBg = (Color){228, 196, 76, 255};     // Golden parchment background
  Color parchmentShadow = (Color){195, 160, 50, 255}; // Rolled edge shadow
  Color parchmentBorder = (Color){70, 45, 10, 255};   // Dark brown ink outline
  Color inkColor = (Color){40, 20, 5, 255};           // Primary ink text
  Color inkSubtle = (Color){90, 60, 20, 255};         // Secondary ink text

  // Main parchment body
  DrawRectangle((int)hudX, (int)hudY, (int)hudWidth, (int)hudHeight, parchmentBg);

  // Top and bottom rolled edges
  DrawRectangle((int)hudX, (int)hudY, (int)hudWidth, 14, parchmentShadow);
  DrawRectangle((int)hudX, (int)(hudY + hudHeight - 14), (int)hudWidth, 14, parchmentShadow);
  DrawLineEx((Vector2){hudX, hudY + 14}, (Vector2){hudX + hudWidth, hudY + 14}, 2.0f, parchmentBorder);
  DrawLineEx((Vector2){hudX, hudY + hudHeight - 14}, (Vector2){hudX + hudWidth, hudY + hudHeight - 14}, 2.0f, parchmentBorder);

  // Scroll corner ears/knobs
  DrawCircle((int)hudX + 8, (int)hudY + 7, 6.0f, parchmentBorder);
  DrawCircle((int)(hudX + hudWidth - 8), (int)hudY + 7, 6.0f, parchmentBorder);
  DrawCircle((int)hudX + 8, (int)(hudY + hudHeight - 7), 6.0f, parchmentBorder);
  DrawCircle((int)(hudX + hudWidth - 8), (int)(hudY + hudHeight - 7), 6.0f, parchmentBorder);

  // Inner decorative border
  DrawRectangleLinesEx((Rectangle){hudX + 8, hudY + 16, hudWidth - 16, hudHeight - 32}, 2.0f, parchmentBorder);

  // --- 2. INTERNAL SAFE MARGINS (PREVENT OVERLAP WITH FRAME) ---
  float safeLeft = hudX + 45.0f; // X = 95px (Wide left margin)
  float safeTop = hudY + 28.0f;  // Y = 448px (Padded from top scroll edge)

  // --- 3. HUD CONTENT & TEXT LAYOUT ---
  // Column 1: Room Info (Left)
  DrawText(room->name, (int)safeLeft, (int)safeTop, 20, inkColor);
  DrawText(TextFormat("Room: %d / %d", game->currentRoom + 1, game->roomCount),
           (int)safeLeft, (int)safeTop + 28, 14, inkSubtle);
  DrawText(TextFormat("Doors in room: %d", room->doorCount),
           (int)safeLeft, (int)safeTop + 48, 14, inkSubtle);

  // Column 2: Control & Servant Status (Center)
  float col2X = safeLeft + 230.0f; // X = 325px
  DrawText("CONTROL MODE", (int)col2X, (int)safeTop, 12, inkSubtle);

  if (game->controlMode == CONTROL_MAROC) {
    DrawText("MAROC (Mage)", (int)col2X, (int)safeTop + 16, 16, (Color){0, 110, 20, 255});
    DrawText("[S] Switch to Servant", (int)col2X, (int)safeTop + 36, 12, inkSubtle);
  } else {
    DrawText("SERVANT (Trance)", (int)col2X, (int)safeTop + 16, 16, (Color){160, 40, 0, 255});
    DrawText("[S] Return to Maroc", (int)col2X, (int)safeTop + 36, 12, inkSubtle);
  }

  if (game->servant.carriedItemId != 0) {
    const char *itemName = (game->servant.carriedItem.name != NULL) ? game->servant.carriedItem.name : "Item";
    DrawText(TextFormat("Servant: Carrying %s", itemName),
             (int)col2X, (int)safeTop + 56, 12, (Color){20, 70, 140, 255});
  } else {
    DrawText("Servant: Active (Empty hands)", (int)col2X, (int)safeTop + 56, 12, (Color){20, 70, 140, 255});
  }

  // Column 3: Maroc Inventory (Right)
  float invStartX = safeLeft + 450.0f; // X = 545px (Ends at X = 689px)
  DrawText("MAROC INVENTORY", (int)invStartX, (int)safeTop, 12, inkSubtle);

  for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
    Rectangle slotRect = {invStartX + i * 48.0f, safeTop + 20.0f, 42.0f, 42.0f};

    // Parchment inventory slot boxes with dark ink border
    DrawRectangleRec(slotRect, (Color){248, 228, 150, 255});
    DrawRectangleLinesEx(slotRect, 2.0f, (i == 0) ? (Color){160, 40, 0, 255} : parchmentBorder);

    if (i < game->player.inventoryCount) {
      const Item *invItem = &game->player.inventory[i];
      Vector2 iconCenter = {slotRect.x + 21.0f, slotRect.y + 21.0f};
      DrawItemSprite(invItem->category, iconCenter, 0.95f, invItem->color);
    }
  }
}

void DrawFallingItems(const Game *game) {
  for (int k = 0; k < MAX_FALLING_ITEMS; k++) {
    const FallingItem *fi = &game->fallingItems[k];
    if (!fi->active)
      continue;

    // 1. Calculate dynamic camera-relative floor shadow position
    Vector2 shadowFloorProj = ProjectRoomPoint(fi->targetLogicalX, fi->targetLogicalY, game->rooms[game->currentRoom].width);
    float shadowScreenX = shadowFloorProj.x + game->cameraScroll.x;
    float shadowScreenY = shadowFloorProj.y + game->cameraScroll.y;

    // 2. Fall progress (0.0 in air -> 1.0 on floor ground)
    float progress = 1.0f;
    if (fi->startY > 0.01f) {
      progress = 1.0f - (fi->currentScreenY / fi->startY);
    }
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    // 3. Shadow expanding & darkening as item approaches floor
    float shadowRadiusX = 4.0f + progress * 3.5f;
    float shadowRadiusY = (4.0f + progress * 3.5f) * 0.5f;
    unsigned char alpha = (unsigned char)(30 + progress * 55);
    DrawEllipse((int)shadowScreenX, (int)shadowScreenY,
                shadowRadiusX, shadowRadiusY, (Color){255, 255, 255, alpha});

    // 4. Sprite is rendered dynamically relative to cameraScroll, floating currentScreenY pixels above shadow
    Vector2 spritePos = {shadowScreenX, shadowScreenY - fi->currentScreenY};
    DrawItemSprite(fi->item.category, spritePos, 0.85f, fi->item.color);
  }
}
