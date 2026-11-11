#include "render.h"
#include "projection.h"
#include <math.h>

void DrawFloorGrid(const Room *room, Vector2 scroll) {
  for (float x = 0; x <= room->width; x += TILE_SIZE) {
    Vector2 p1 = ProjectRoomPoint(x, 0.0f, room->width);
    Vector2 p2 = ProjectRoomPoint(x, room->length, room->width);
    DrawLine(p1.x + scroll.x, p1.y + scroll.y, p2.x + scroll.x, p2.y + scroll.y,
             DARKGRAY);
  }

  for (float y = 0; y <= room->length; y += TILE_SIZE) {
    Vector2 p1 = ProjectRoomPoint(0.0f, y, room->width);
    Vector2 p2 = ProjectRoomPoint(room->width, y, room->width);
    DrawLine(p1.x + scroll.x, p1.y + scroll.y, p2.x + scroll.x, p2.y + scroll.y,
             DARKGRAY);
  }
}

void DrawWalls(const Room *room, Vector2 scroll) {
  Vector2 leftBottom = ProjectRoomPoint(0.0f, 0.0f, room->width);
  Vector2 leftTop = ProjectRoomPoint(0.0f, room->length, room->width);
  Vector2 rightTop = ProjectRoomPoint(room->width, room->length, room->width);
  Vector2 rightBottom = ProjectRoomPoint(room->width, 0.0f, room->width);

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
    Vector2 knobPos = {doorBase.x + (door->width * 0.25f), // Knob
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

  float leafLength = door->width * 0.90f;      // Value controls leaf length
  const float MAX_ANGLE = (PI / 2.0f) * 0.88f; // Value controls leaf angle
  float angle = door->openProgress * MAX_ANGLE;

  // Thsese numbers must also change to adjust when leaf length/angle is changed
  float dx = leafLength * cosf(angle) - (leafLength * 0.55f) * sinf(angle);
  float dy = (leafLength * 0.38f) * sinf(angle);

  Vector2 edgeBottom = {hingeBottom.x + dx, hingeBottom.y + dy};
  Vector2 edgeTop = {edgeBottom.x, edgeBottom.y - door->height};

  Color doorLeafColor = {25, 25, 25, 255};
  DrawTriangle(hingeBottom, hingeTop, edgeBottom, doorLeafColor);
  DrawTriangle(hingeTop, edgeTop, edgeBottom, doorLeafColor);

  DrawLineEx(hingeBottom, edgeBottom, 2.5f, door->color);
  DrawLineEx(edgeBottom, edgeTop, 2.5f, door->color);
  DrawLineEx(edgeTop, hingeTop, 2.5f, door->color);

  const float handleRatio = 0.85f; // Knob
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
  float xWall = (door->wall == WALL_LEFT) ? 1.0f : (room->width - 1.0f);

  Vector2 baseFront = ProjectRoomPoint(xWall, yFront, room->width);
  Vector2 baseBack = ProjectRoomPoint(xWall, yBack, room->width);
  baseFront.x += scroll.x;
  baseFront.y += scroll.y;
  baseBack.x += scroll.x;
  baseBack.y += scroll.y;

  Vector2 roofFront = {baseFront.x, baseFront.y - door->height};
  Vector2 roofBack = {baseBack.x, baseBack.y - door->height};

  if (door->state == DOOR_FREE_EXIT) {
    DrawTriangle(baseFront, baseBack, roofFront, BLACK);
    DrawTriangle(roofFront, baseBack,
                 (Vector2){baseBack.x, baseBack.y - door->height}, BLACK);
    DrawLineEx(baseFront, roofFront, 2.0f, door->color);
    DrawLineEx(roofFront, (Vector2){baseBack.x, baseBack.y - door->height},
               2.0f, door->color);
    DrawLineEx((Vector2){baseBack.x, baseBack.y - door->height}, baseBack, 2.0f,
               door->color);
    return;
  }

  Color fillColor = (door->state == DOOR_CLOSED && door->openProgress <= 0.0f)
                        ? DARKGRAY
                        : BLACK;

  if (door->wall == WALL_LEFT) {
    DrawTriangle(baseFront, baseBack, roofFront, fillColor);
    DrawTriangle(roofFront, baseBack, roofBack, fillColor);
  } else {
    DrawTriangle(baseFront, roofFront, baseBack, fillColor);
    DrawTriangle(roofFront, roofBack, baseBack, fillColor);
  }

  DrawLineEx(baseFront, roofFront, 2.0f, door->color);
  DrawLineEx(roofFront, roofBack, 2.0f, door->color);
  DrawLineEx(roofBack, baseBack, 2.0f, door->color);
  DrawLineEx(baseBack, baseFront, 2.0f, door->color);

  if (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) {
    float centerX = (baseFront.x + baseBack.x) / 2.0f;
    float centerY = (baseFront.y + baseBack.y) / 2.0f - door->height / 2.0f;
    DrawText("X", centerX - 4, centerY - 5, 12, RED);

    float handleAltitude = door->height * 0.45f; // Knob
    Vector2 knobPos = {baseFront.x + (baseBack.x - baseFront.x) * 0.20f,
                       (baseFront.y + (baseBack.y - baseFront.y) * 0.20f) -
                           handleAltitude};
    DrawCircleV(knobPos, 3.0f, GOLD);
    DrawCircleLines((int)knobPos.x, (int)knobPos.y, 3.5f, DARKBROWN);
  } else if (door->openProgress > 0.0f) {
    Vector2 hingeBottom = baseBack;
    Vector2 hingeTop = roofBack;

    Vector2 doorwayVector = {baseFront.x - baseBack.x,
                             baseFront.y - baseBack.y};
    float doorwayPixelSpan = sqrtf(doorwayVector.x * doorwayVector.x +
                                   doorwayVector.y * doorwayVector.y);
    float leafLength =
        doorwayPixelSpan *
        1.1f; // Put a bigger number (now is 110%) for a larger open door

    Vector2 wallDir = {0.0f, 0.0f};
    if (doorwayPixelSpan > 0.0f) {
      wallDir.x = doorwayVector.x / doorwayPixelSpan;
      wallDir.y = doorwayVector.y / doorwayPixelSpan;
    }

    // Rotation angle from 0 to PI/2
    float angle = door->openProgress * (PI / 2.0f);

    // Normal vector pointing toward the interior of the room
    float inwardX = (door->wall == WALL_LEFT) ? 1.0f : -1.0f;

    Vector2 edgeBottom;
    edgeBottom.x = hingeBottom.x + (wallDir.x * leafLength * cosf(angle)) +
                   (inwardX * leafLength * sinf(angle));
    edgeBottom.y = hingeBottom.y + (wallDir.y * leafLength * cosf(angle));

    Vector2 edgeTop = {edgeBottom.x, edgeBottom.y - door->height};

    Color doorLeafColor = {25, 25, 25, 255};

    if (door->wall == WALL_LEFT) {
      DrawTriangle(hingeBottom, edgeBottom, hingeTop, doorLeafColor);
      DrawTriangle(hingeTop, edgeBottom, edgeTop, doorLeafColor);
    } else {
      DrawTriangle(hingeBottom, hingeTop, edgeBottom, doorLeafColor);
      DrawTriangle(hingeTop, edgeTop, edgeBottom, doorLeafColor);
    }

    DrawLineEx(hingeBottom, edgeBottom, 2.5f, door->color);
    DrawLineEx(edgeBottom, edgeTop, 2.5f, door->color);
    DrawLineEx(edgeTop, hingeTop, 2.5f, door->color);

    const float handleRatio = 0.90f; // Knob
    float handleAltitude = door->height * 0.45f;
    Vector2 knobPos = {
        hingeBottom.x + (edgeBottom.x - hingeBottom.x) * handleRatio,
        (hingeBottom.y + (edgeBottom.y - hingeBottom.y) * handleRatio) -
            handleAltitude};
    DrawCircleV(knobPos, 3.0f, GOLD);
    DrawCircleLines((int)knobPos.x, (int)knobPos.y, 3.5f, DARKBROWN);
  }
}

static bool IsDoorInFrontOfPlayer(const Door *door, const Room *room,
                                  const Player *player) {
  if (door->wall == WALL_BACK) {
    // Only occludes if the player has physically moved past the back wall,
    // entering the space of the outer tunnel.
    return player->y > room->length;
  }

  // Side doors (WALL_LEFT and WALL_RIGHT):
  float yBack = door->position + (door->width / 2.0f);
  float leafSpan = door->height * 0.75f;

  // It only occludes if it is behind the hinge and flush against that wall
  if (door->wall == WALL_LEFT) {
    bool behindInY = (player->y > (yBack - 6.0f));
    bool nearWallX = (player->x >= 0.0f && player->x <= (leafSpan + 4.0f));
    return (behindInY && nearWallX && door->openProgress > 0.0f);
  } else if (door->wall == WALL_RIGHT) {
    bool behindInY = (player->y > (yBack - 6.0f));
    bool nearWallX = (player->x >= (room->width - leafSpan - 4.0f) &&
                      player->x <= room->width);
    return (behindInY && nearWallX && door->openProgress > 0.0f);
  }

  return false;
}

static bool IsLeafInFrontOfPlayer(const Door *door, const Room *room,
                                  const Player *player) {
  if (door->state == DOOR_FREE_EXIT || door->openProgress <= 0.0f)
    return false;

  // Back door: only obscures the view if the player walks right against the
  // wall behind the door leaf
  if (door->wall == WALL_BACK) {
    float hingeX = door->position - (door->width / 2.0f);
    bool behindInY = (player->y >= (room->length - 12.0f));
    bool behindInX =
        (player->x <= hingeX && player->x >= (hingeX - door->width * 0.6f));
    return (behindInY && behindInX);
  }

  // Side doors (WALL_LEFT and WALL_RIGHT):
  // The hinge and the base of the door leaf are at yBack
  float yBack = door->position + (door->width / 2.0f);
  float leafSpan = door->height * 0.75f;

  // CRITICAL CORRECTION:
  // The leaf can ONLY cover the character if the player's center has
  // physically moved past the leaf's line toward the back of the room
  // (player->y > yBack). If the player collides from below (player->y <=
  // yBack), the leaf remains BEHIND Maroc.
  if (door->wall == WALL_LEFT) {
    bool behindInY =
        (player->y > yBack); // Removed the cautionary margin (- 6.0f)
    bool nearWallX = (player->x >= 0.0f && player->x <= (leafSpan + 4.0f));
    return (behindInY && nearWallX);
  } else if (door->wall == WALL_RIGHT) {
    bool behindInY =
        (player->y > yBack); // Removed the cautionary margin (- 6.0f)
    bool nearWallX = (player->x >= (room->width - leafSpan - 4.0f) &&
                      player->x <= room->width);
    return (behindInY && nearWallX);
  }

  return false;
}

void DrawDoorsBehind(const Room *room, const Player *player, Vector2 scroll) {
  for (int i = 0; i < room->doorCount; i++) {
    const Door *door = &room->doors[i];

    if (door->wall == WALL_BACK) {
      // The black void in the background is ALWAYS drawn behind the player
      DrawBackDoorHole(door, room, scroll);
      // If the leaf is not in front, it is drawn here
      if (!IsLeafInFrontOfPlayer(door, room, player)) {
        DrawBackDoorLeaf(door, room, scroll);
      }
    } else {
      // For sides, if it is not in front, it is drawn in its entirety
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
        // Only draw the wooden board, NEVER the black rectangle
        DrawBackDoorLeaf(door, room, scroll);
      } else {
        DrawSideDoor(door, room, scroll);
      }
    }
  }
}

void DrawPlayer(const Game *game) {
  Vector2 shadowPos = {game->projectedPlayer.x + game->cameraScroll.x,
                       game->projectedPlayer.y + game->cameraScroll.y};

  // Semi-transparent floor shadow oval
  DrawEllipse(shadowPos.x, shadowPos.y, SHADOW_RADIUS_X, SHADOW_RADIUS_Y,
              (Color){255, 255, 255, 85});

  // Levitating character cube floating above the shadow
  float totalAltitude = game->player.z + game->player.floatingOffset;
  DrawRectangle(shadowPos.x - PLAYER_WIDTH / 2.0f,
                shadowPos.y - PLAYER_HEIGHT - totalAltitude, PLAYER_WIDTH,
                PLAYER_HEIGHT, WHITE);
}

// =============================================================================
// RETRO ITEM SPRITE RENDERING (src/render.c)
// =============================================================================
static void DrawItemSprite(ItemCategory category, Vector2 centerPos,
                           float scale, Color color) {
  switch (category) {
  case ITEM_TYPE_KEY: {  // Retro key: bow + shank + bit
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
  case ITEM_TYPE_SCROLL: {  // Rolled-up parchment
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
  case ITEM_TYPE_ARTIFACT: {   // Artefacto mágico

    DrawCircleV(centerPos, 6.0f * scale, color);
    DrawCircleLines((int)centerPos.x, (int)centerPos.y, 8.0f * scale, RAYWHITE);
    break;
  }
  }
}

void DrawHUD(const Game *game) {
    const Room *room = &game->rooms[game->currentRoom];
    DrawText(room->name, 20, 20, 20, YELLOW);
    DrawText(TextFormat("Room: %d | Doors: %d", game->currentRoom + 1, room->doorCount), 20, 48, 14, RAYWHITE);

    // --- MAROC'S INVENTORY (3 SLOTS ON SCREEN) ---
    int startX = SCREEN_WIDTH - 190;
    int startY = SCREEN_HEIGHT - 45;

    DrawText("INVENTORY", startX, startY - 16, 10, LIGHTGRAY);

    for (int i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        Rectangle slotRect = { (float)(startX + i * 55), (float)startY, 50.0f, 35.0f };
        DrawRectangleRec(slotRect, (Color){ 20, 20, 20, 200 });
        DrawRectangleLinesEx(slotRect, 1.5f, (i == 0) ? GOLD : GRAY); // Casilla 0 resaltada (siguiente en salir)

        if (i < game->player.inventoryCount) {
            const Item *invItem = &game->player.inventory[i];
            Vector2 iconCenter = { slotRect.x + 25.0f, slotRect.y + 17.0f };
            DrawItemSprite(invItem->category, iconCenter, 1.1f, invItem->color);
        }
    }
}

void DrawServant(const Servant *s, float roomWidth, Vector2 scroll,
                 const Game *game) {
  (void)roomWidth;
  (void)scroll;
  (void)game;

  if (!s->active)
    return;

  // Glowing energy pulse
  float pulse = sinf((float)GetTime() * 14.0f);
  Color coreColor =
      (pulse > 0.0f) ? (Color){255, 105, 180, 255} : (Color){255, 20, 147, 255};

  // Draw strictly at the current screen coordinates (no offsets, no
  // reprojection)
  Vector2 orbCenter = {s->screenX, s->screenY};
  DrawCircleV(orbCenter, 4.5f, coreColor);
  DrawCircleLines((int)orbCenter.x, (int)orbCenter.y, 6.5f,
                  (Color){255, 192, 203, 190});

  if (s->carriedItemId != 0) {
    // DrawItemMiniIcon(s->carriedItemId, (Vector2){orbCenter.x, orbCenter.y
    // - 10.0f}, game);
  }
}

void DrawRoomItems(const Room *room, Vector2 scroll) {
    float time = (float)GetTime();

    for (int i = 0; i < room->itemCount; i++) {
        const Item *item = &room->items[i];
        if (!item->active) continue;

        if (!item->isWallItem) {
           // --- OBJECT ON THE GROUND ---
            Vector2 floorPos = ProjectRoomPoint(item->x, item->y, room->width);
            floorPos.x += scroll.x;
            floorPos.y += scroll.y;

            // Shadow on the ground
            DrawEllipse((int)floorPos.x, (int)floorPos.y, 8, 4, (Color){ 0, 0, 0, 100 });

            // Gentle floating (bobbing)
            float hoverOffset = sinf(time * 3.0f + item->id) * 3.0f;
            Vector2 itemCenter = { floorPos.x, floorPos.y - 8.0f - hoverOffset };

            DrawItemSprite(item->category, itemCenter, 1.0f, item->color);
        } else {
            // --- OBJECT ON THE WALL (Wall projection) ---
            Vector2 wallBase = ProjectRoomPoint(item->x, item->y, room->width);
            wallBase.x += scroll.x;
            wallBase.y += scroll.y - item->wallHeightOffset;

            DrawItemSprite(item->category, wallBase, 1.0f, item->color);
            // Gleam on wall
            DrawCircleLines((int)wallBase.x, (int)wallBase.y, 10.0f, (Color){ 255, 255, 255, 90 });
        }
    }
}


