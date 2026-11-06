#include "physics.h"
#include "config.h"
#include "projection.h"
#include <math.h>

void ApplyBounce(Player *player, float forceX, float forceY) {
  player->bounceForceX = forceX;
  player->bounceForceY = forceY;
  player->bounceLockoutTimer = BOUNCE_LOCKOUT_TIME;
}

void UpdatePlayerPhysics(Game *game, float *outMoveX, float *outMoveY,
                         float dt) {
  Player *p = &game->player;
  Room *r = &game->rooms[game->currentRoom];

  float moveX = 0.0f;
  float moveY = 0.0f;

  // Levitation smooth floating oscillation (bobbing)
  p->levitationTimer += dt * 4.0f;
  p->floatingOffset = sinf(p->levitationTimer) * 3.5f;

  // Visual unit ratio to ensure identical screen movement speed across all room
  // sizes
  Vector2 basePoint = ProjectRoomPoint(0.0f, p->y, r->width);
  Vector2 testPoint = ProjectRoomPoint(1.0f, p->y, r->width);
  float pixelsPerLogicalUnit = fabsf(testPoint.x - basePoint.x);

  // If bouncing, player has no input control; smooth friction takes over
  if (p->bounceLockoutTimer > 0.0f) {
    p->bounceLockoutTimer -= dt;

    float bounceStepX = 0.0f;
    if (pixelsPerLogicalUnit > 0.0f) {
      bounceStepX = (p->bounceForceX * dt) / pixelsPerLogicalUnit;
    }

    p->x += bounceStepX;
    p->y += p->bounceForceY * dt;

    p->bounceForceX *= BOUNCE_FRICTION;
    p->bounceForceY *= BOUNCE_FRICTION;
  } else {
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
      moveX += 1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
      moveX -= 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
      moveY += 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
      moveY -= 1.0f;

    if (moveX != 0.0f || moveY != 0.0f) {
      if (moveX != 0.0f && moveY != 0.0f) {
        moveX *= DIAGONAL_FACTOR_X;
      }
      float length = sqrtf(moveX * moveX + moveY * moveY);
      moveX /= length;
      moveY /= length;

      float stepX = 0.0f;
      if (pixelsPerLogicalUnit > 0.0f) {
        stepX = (moveX * (p->speed * SPEED_MULTIPLIER_X) * dt) /
                pixelsPerLogicalUnit;
      }
      float stepY = moveY * p->speed * dt;

      p->x += stepX;
      p->y += stepY;
    }
  }

  *outMoveX = moveX;
  *outMoveY = moveY;
}

#define SERVANT_BODY_RADIUS 8.0f // Radio físico de su cuerpo luminoso

// Interaction upon moving one's body over Maroc (picks up the active object)
void CheckServantTakeFromMaroc(Servant *s, Player *p) {
  if (!s->active || s->carriedItemId != 0)
    return;

  // Distance between the servant's body and Maroc's shadow/base
  float dx = s->x - p->x;
  float dy = s->y - p->y;
  float dist = sqrtf(dx * dx + dy * dy);

  // When the servant's body comes into contact with Maroc
  if (dist <= (SERVANT_BODY_RADIUS + 8.0f)) {
    if (p->inventoryCount > 0) {
      // Takes the first item from the inventory
      s->carriedItemId = p->inventory[0].id;
    }
  }
}

void UpdateServantItemInteraction(Servant *s, Room *r) {
  if (!s->active)
    return;

  for (int i = 0; i < r->itemCount; i++) {
    Item *item = &r->items[i];
    if (!item->active)
      continue;

    float dx = item->x - s->x;
    float dy = item->y - s->y;
    float distSq = dx * dx + dy * dy;

    // Direct physical contact with the servant’s body
    if (distSq <= (SERVANT_BODY_RADIUS * SERVANT_BODY_RADIUS)) {
      // Si el sirviente va desarmado, recoge el objeto del suelo
      if (s->carriedItemId == 0) {
        item->active = false;
        s->carriedItemId = item->id;
      }
      // If it is already carrying an object, the reaction/fusion between
      // both will be checked here
    }
  }
}

void UpdateServantPhysics(Game *game, float dt) {
  Servant *s = &game->servant;
  Room *r = &game->rooms[game->currentRoom];

  if (!s->active)
    return;

  float moveX = 0.0f;
  float moveY = 0.0f;

  if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
    moveX += 1.0f;
  if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
    moveX -= 1.0f;
  if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
    moveY -= 1.0f; // En pantalla, Arriba es -Y
  if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
    moveY += 1.0f; 

  // If there is movement, we move the servant within the direct visual plane
  if (moveX != 0.0f || moveY != 0.0f) {
    float length = sqrtf(moveX * moveX + moveY * moveY);
    moveX /= length;
    moveY /= length;

    Vector2 screenPos = ProjectRoomPoint(s->x, s->y, r->width);
    screenPos.x += game->cameraScroll.x;
    screenPos.y += game->cameraScroll.y;

    screenPos.x += moveX * s->speed * dt;
    screenPos.y += moveY * s->speed * dt;

    // 3. Comprobación de salida de la ventana (retorno a Maroc)
    const float margin = 20.0f;
    if (screenPos.x < -margin || screenPos.x > (SCREEN_WIDTH + margin) ||
        screenPos.y < -margin || screenPos.y > (SCREEN_HEIGHT + margin)) {
      s->active = false;
      game->controlMode = CONTROL_MAROC;
      return;
    }

    // Unprojection: convert screen pixels to logical coordinates
    // (s->x, s->y) pos.y = -(y * 0.45f) * 1.3f => y = -pos.y / (0.45f
    // * 1.3f)
    float localY = -(screenPos.y - game->cameraScroll.y) / (0.45f * 1.3f);
    s->y = localY;

    // Compute lateral margins of the trapezoid at that height Y
    float factorY = s->y / FIXED_VISUAL_DEPTH;
    if (factorY < 0.0f)
      factorY = 0.0f;
    if (factorY > 1.0f)
      factorY = 1.0f;

    float xFrenteIzq = -310.0f;
    float xFondoIzq = -130.0f;
    float limIzqPantalla =
        (xFrenteIzq + (xFondoIzq - xFrenteIzq) * factorY) * 1.3f;

    float xFrenteDer = r->width + 310.0f;
    float xFondoDer = r->width + 130.0f;
    float limDerPantalla =
        (xFrenteDer + (xFondoDer - xFrenteDer) * factorY) * 1.3f;

    float anchoVisible = limDerPantalla - limIzqPantalla;
    float localX = screenPos.x - game->cameraScroll.x;

    if (anchoVisible > 0.0f) {
      float factorX = (localX - limIzqPantalla) / anchoVisible;
      s->x = factorX * r->width;
    }
  }

  // Tactile interactions of the servant with its body
  CheckServantTakeFromMaroc(s, &game->player);
  UpdateServantItemInteraction(s, r);
}

void TransitionToRoom(Game *game, const Door *crossedDoor) {
  int nextRoomIndex = crossedDoor->targetRoom;
  int targetDoorId = crossedDoor->targetDoorId;
  Room *nextRoom = &game->rooms[nextRoomIndex];

  for (int i = 0; i < nextRoom->doorCount; i++) {
    Door *dest = &nextRoom->doors[i];
    if (dest->id == targetDoorId) {
      if (dest->wall == WALL_LEFT) {
        game->player.x = 8.0f; // A step inward
        game->player.y = dest->position;
      } else if (dest->wall == WALL_RIGHT) {
        game->player.x = nextRoom->width - 8.0f; // A step inward
        game->player.y = dest->position;
      } else if (dest->wall == WALL_BACK) {
        game->player.x = dest->position;
        // We shift the position toward the front of the room so that it does
        // not end up buried beneath the lintel or inside the black frame
        game->player.y = nextRoom->length - 30.0f;
      }
      break;
    }
  }

  game->currentRoom = nextRoomIndex;
}

void UpdateCollisions(Game *game, float prevX, float prevY, float moveX,
                      float moveY) {
  Player *p = &game->player;
  Room *r = &game->rooms[game->currentRoom];

  bool hasChangedRoom = false;
  Door crossedDoor;

  // --- 1. LEFT WALL COLLISION ---
  if (p->x < 8.0f) {
    bool insideDoorway = false;
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_LEFT) {
        float centerMarginY = d->width / 4.0f;
        if (p->y >= (d->position - centerMarginY) &&
            p->y <= (d->position + centerMarginY)) {
          if (d->state == DOOR_CLOSED) {
            d->state = DOOR_OPENING;
            ApplyBounce(p, BOUNCE_IMPULSE_FORCE, 0.0f);
            p->x = 0.0f;
            insideDoorway = false;
          } else {
            insideDoorway = true;
            if (p->x < -3.0f) {
              crossedDoor = *d;
              hasChangedRoom = true;
            }
          }
          break;
        }
      }
    }
    if (!insideDoorway && p->x < 0.0f)
      p->x = 0.0f;
  }

  // --- 2. RIGHT WALL COLLISION ---
  if (p->x > (r->width - 8.0f) && !hasChangedRoom) {
    bool insideDoorway = false;
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_RIGHT) {
        float centerMarginY = d->width / 4.0f;
        if (p->y >= (d->position - centerMarginY) &&
            p->y <= (d->position + centerMarginY)) {
          if (d->state == DOOR_CLOSED) {
            d->state = DOOR_OPENING;
            ApplyBounce(p, -BOUNCE_IMPULSE_FORCE, 0.0f);
            p->x = r->width;
            insideDoorway = false;
          } else {
            insideDoorway = true;
            if (p->x > (r->width + 3.0f)) {
              crossedDoor = *d;
              hasChangedRoom = true;
            }
          }
          break;
        }
      }
    }
    if (!insideDoorway && p->x > r->width)
      p->x = r->width;
  }

  // --- 3. BACK WALL COLLISION ---
  if (p->y > r->length && !hasChangedRoom) {
    bool insideDoorway = false;
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_BACK) {
        float centerMarginX = d->width / 4.0f;
        if (p->x >= (d->position - centerMarginX) &&
            p->x <= (d->position + centerMarginX)) {
          if (d->state == DOOR_CLOSED) {
            d->state = DOOR_OPENING;
            ApplyBounce(p, 0.0f, -BOUNCE_IMPULSE_FORCE);
            p->y = r->length;
            insideDoorway = false;
          } else {
            insideDoorway = true;
            if (p->y > (r->length + 8.0f)) {
              crossedDoor = *d;
              hasChangedRoom = true;
            }
          }
          break;
        }
      }
    }
    if (!insideDoorway)
      p->y = r->length;
  }

  // --- 4. BLOCK SIDE DOORS IF PASSING FROM BELOW (moveY > 0) ---
  if (moveY > 0.0f) {
    Vector2 screenShadow = ProjectRoomPoint(p->x, p->y, r->width);
    screenShadow.x += game->cameraScroll.x;
    screenShadow.y += game->cameraScroll.y;

    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->state == DOOR_OPEN &&
          (d->wall == WALL_LEFT || d->wall == WALL_RIGHT)) {
        // Position of the leaf line on the floor (rear frame where the hinge is
        // located)
        float leafY = d->position + (d->width / 2.0f);
        const float shadowFloorY = SHADOW_RADIUS_Y / 1.55f;

        // Hinge position on screen
        float wallX = (d->wall == WALL_LEFT) ? 1.0f : (r->width - 1.0f);
        Vector2 rearBase = ProjectRoomPoint(wallX, leafY, r->width);
        rearBase.x += game->cameraScroll.x;

        float leafWidthPixels = d->height * 0.75f;

        // Horizontal screen check (matches DrawSideDoor rendering)
        bool inLeafRangeScreen = false;
        if (d->wall == WALL_LEFT) {
          float leafTipX = rearBase.x + leafWidthPixels;
          inLeafRangeScreen = (screenShadow.x <= leafTipX);
        } else if (d->wall == WALL_RIGHT) {
          float leafTipX = rearBase.x - leafWidthPixels;
          inLeafRangeScreen = (screenShadow.x >= leafTipX);
        }

        if (inLeafRangeScreen) {
          // ompute position of the top edge of the shadow's ellipse
          float prevShadowTop = prevY + shadowFloorY;
          float currShadowTop = p->y + shadowFloorY;

          // Contact is detected as soon as the TOP part of the shadow
          // reaches the wood
          if (prevShadowTop <= leafY && currShadowTop >= leafY) {
            // Position the top part of the ellipse exactly on the line
            // on the sheet
            p->y = leafY - shadowFloorY;
          }
        }
      }
    }
  }

  // --- 5. CLOSE SIDE DOORS VIA VERTICAL PUSH (moveY < 0) ---
  if (moveY < 0.0f) {
    Vector2 screenShadow = ProjectRoomPoint(p->x, p->y, r->width);
    screenShadow.x += game->cameraScroll.x;
    screenShadow.y += game->cameraScroll.y;

    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->state == DOOR_OPEN &&
          (d->wall == WALL_LEFT || d->wall == WALL_RIGHT)) {
        float leafY = d->position + (d->width / 2.0f);
        const float shadowFloorY = SHADOW_RADIUS_Y / 1.55f;

        float wallX = (d->wall == WALL_LEFT) ? 1.0f : (r->width - 1.0f);
        Vector2 rearBase = ProjectRoomPoint(wallX, leafY, r->width);
        rearBase.x += game->cameraScroll.x;

        float leafWidthPixels = d->height * 0.75f;

        bool inLeafRangeScreen = false;
        if (d->wall == WALL_LEFT) {
          float leafTipX = rearBase.x + leafWidthPixels;
          inLeafRangeScreen = (screenShadow.x <= leafTipX);
        } else if (d->wall == WALL_RIGHT) {
          float leafTipX = rearBase.x - leafWidthPixels;
          inLeafRangeScreen = (screenShadow.x >= leafTipX);
        }

        if (inLeafRangeScreen && prevY >= leafY) {
          float prevShadowBottom = prevY - shadowFloorY;
          float currShadowBottom = p->y - shadowFloorY;

          if (prevShadowBottom >= leafY && currShadowBottom <= leafY) {
            d->state = DOOR_CLOSING;
            p->y = leafY + shadowFloorY;
          }
        }
      }
    }
  }

  // --- 6. BLOCK BACK DOORS IF PASSING FROM right (moveX < 0) ---
  if (moveX < 0.0f) {
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_BACK && d->state == DOOR_OPEN) {
        // Hinge position on the back wall
        float hingeX = d->position - (d->width / 2.0f);
        float leafFrontY = r->length - (d->width * 0.35f);

        // Check if the player is in the depth band where the leaf exists
        if (p->y >= (leafFrontY - 5.0f) && p->y <= (r->length + 4.0f)) {
          // If coming from the right of the hinge, block leftward movement
          if (prevX >= hingeX && (p->x - SHADOW_RADIUS_X) <= hingeX) {
            p->x = hingeX + SHADOW_RADIUS_X; // Solid obstacle stop
            break;
          }
        }
      }
    }
  }
  // --- 7. CLOSE BACK DOOR VIA HORIZONTAL PUSH (moveX > 0) ---
  if (moveX > 0.0f) {
    Vector2 projectedPos = ProjectRoomPoint(p->x, p->y, r->width);
    Vector2 screenShadowPos = {projectedPos.x + game->cameraScroll.x,
                               projectedPos.y + game->cameraScroll.y};

    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_BACK && d->state == DOOR_OPEN) {
        Vector2 doorBase = ProjectRoomPoint(d->position, r->length, r->width);
        doorBase.x += game->cameraScroll.x;
        doorBase.y += game->cameraScroll.y;

        Vector2 hingeBottom = {doorBase.x - d->width / 2.0f, doorBase.y};
        Vector2 edgeBottom = {doorBase.x - d->width / 2.0f - d->width * 0.75f,
                              doorBase.y + d->height * 0.25f};

        float leafCenterX = (hingeBottom.x + edgeBottom.x) / 2.0f;
        float leafCenterY = (hingeBottom.y + edgeBottom.y) / 2.0f;

        float distX = fabsf(screenShadowPos.x - leafCenterX);
        float distY = fabsf(screenShadowPos.y - leafCenterY);

        if (distX < 20.0f && distY < 12.0f) {
          d->state = DOOR_CLOSING;
          p->x = prevX;
          break;
        }
      }
    }
  }

  // --- 6. FRONT ROOM BOUNDARY COLLISION ---
  if (p->y < 0.0f) {
    p->y = prevY;
  }

  // --- 7. FAN PERSPECTIVE COMPLIANCE (ONLY ON NET VERTICAL DISPLACEMENT) ---
  if (moveY != 0.0f && p->y != prevY) {
    float stepY = (p->y - prevY);
    float roomCenter = r->width / 2.0f;
    float deviation = p->x - roomCenter;
    p->x += (deviation * (0.45f / FIXED_VISUAL_DEPTH)) * stepY;
  }

  // --- 8. TRIGGER ROOM SWITCH ---
  if (hasChangedRoom) {
    TransitionToRoom(game, &crossedDoor);
  }
}

void UpdateDoorAnimations(Room *room, float dt) {
  const float DOOR_SPEED = 2.5f; // It takes ~0.4s

  for (int i = 0; i < room->doorCount; i++) {
    Door *d = &room->doors[i];

    if (d->state == DOOR_OPENING) {
      d->openProgress += DOOR_SPEED * dt;
      if (d->openProgress >= 1.0f) {
        d->openProgress = 1.0f;
        d->state = DOOR_OPEN;
      }
    } else if (d->state == DOOR_CLOSING) {
      d->openProgress -= DOOR_SPEED * dt;
      if (d->openProgress <= 0.0f) {
        d->openProgress = 0.0f;
        d->state = DOOR_CLOSED;
      }
    } else if (d->state == DOOR_OPEN) {
      d->openProgress = 1.0f;
    } else if (d->state == DOOR_CLOSED) {
      d->openProgress = 0.0f;
    }
  }
}
