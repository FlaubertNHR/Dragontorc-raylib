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

  float moveX = 0.0f;
  float moveY = 0.0f;

  // Levitation smooth floating oscillation (bobbing)
  p->levitationTimer += dt * LEVITATION_SPEED;
  p->floatingOffset = sinf(p->levitationTimer) * LEVITATION_AMPLITUDE;

  float pixelsPerLogicalUnit = PROJECTION_SCALE;

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
    if (IsKeyDown(KEY_RIGHT))
      moveX += 1.0f;
    if (IsKeyDown(KEY_LEFT))
      moveX -= 1.0f;
    if (IsKeyDown(KEY_UP))
      moveY += 1.0f;
    if (IsKeyDown(KEY_DOWN))
      moveY -= 1.0f;

    if (moveX != 0.0f || moveY != 0.0f) {
      if (moveX != 0.0f && moveY != 0.0f) {
        moveX *= DIAGONAL_FACTOR_X;
      }
      float length = sqrtf(moveX * moveX + moveY * moveY);
      moveX /= length;
      moveY /= length;

      float stepX = (moveX * (p->speed * SPEED_MULTIPLIER_X) * dt) / pixelsPerLogicalUnit;
      float stepY = moveY * p->speed * dt;

      p->x += stepX;
      p->y += stepY;
    }
  }

  *outMoveX = moveX;
  *outMoveY = moveY;
}

void TransitionToRoom(Game *game, const Door *crossedDoor) {
  int nextRoomIndex = crossedDoor->targetRoom;
  int targetDoorId = crossedDoor->targetDoorId;
  Room *nextRoom = &game->rooms[nextRoomIndex];

  for (int i = 0; i < nextRoom->doorCount; i++) {
    Door *dest = &nextRoom->doors[i];
    if (dest->id == targetDoorId) {
      if (dest->wall == WALL_LEFT) {
        float leftWall = GetLeftWallX(dest->position, nextRoom->width, nextRoom->length);
        game->player.x = leftWall + DOOR_TRANSITION_STEP_X; // A step inward
        game->player.y = dest->position;
      } else if (dest->wall == WALL_RIGHT) {
        float rightWall = GetRightWallX(dest->position, nextRoom->width, nextRoom->length);
        game->player.x = rightWall - DOOR_TRANSITION_STEP_X;
        game->player.y = dest->position;
      } else if (dest->wall == WALL_BACK) {
        game->player.x = dest->position;
        game->player.y = nextRoom->length - DOOR_TRANSITION_BACK_MARGIN_Y;
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

  float leftWallX = GetLeftWallX(p->y, r->width, r->length);
  float rightWallX = GetRightWallX(p->y, r->width, r->length);

  const float shadowRadiusX = SHADOW_LOGIC_RADIUS_X;
  const float shadowRadiusY = SHADOW_LOGIC_RADIUS_Y;

  // --- 1. LEFT WALL COLLISION ---
  if ((p->x - shadowRadiusX) < (leftWallX + SIDE_WALL_DETECTION_MARGIN)) {
    bool insideDoorway = false;
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_LEFT) {
        float yFront = d->position - (d->width / 2.0f);
        float yBack = d->position + (d->width / 2.0f);

        // Walkable doorway threshold ends before the rear hinge line
        if (p->y >= yFront && p->y <= yBack) {
          if (d->state == DOOR_CLOSED) {
            d->state = DOOR_OPENING;
            ApplyBounce(p, BOUNCE_IMPULSE_FORCE, 0.0f);
            p->x = leftWallX + shadowRadiusX;
            insideDoorway = false;
          } else {
            insideDoorway = true;
            if (p->x < (leftWallX - DOOR_TRIGGER_THRESHOLD_SIDE)) {
              crossedDoor = *d;
              hasChangedRoom = true;
            }
          }
          break;
        }
      }
    }
    if (!insideDoorway && (p->x - shadowRadiusX) < leftWallX)
      p->x = leftWallX + shadowRadiusX;
  }

  // --- 2. RIGHT WALL COLLISION ---
  if ((p->x + shadowRadiusX) > (rightWallX - SIDE_WALL_DETECTION_MARGIN) && !hasChangedRoom) {
    bool insideDoorway = false;
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_RIGHT) {
        float yFront = d->position - (d->width / 2.0f);
        float yBack = d->position + (d->width / 2.0f);

        // Walkable doorway threshold ends before the rear hinge line
        if (p->y >= yFront && p->y <= yBack) {
          if (d->state == DOOR_CLOSED) {
            d->state = DOOR_OPENING;
            ApplyBounce(p, -BOUNCE_IMPULSE_FORCE, 0.0f);
            p->x = rightWallX - shadowRadiusX;
            insideDoorway = false;
          } else {
            insideDoorway = true;
            if (p->x > (rightWallX + DOOR_TRIGGER_THRESHOLD_SIDE)) {
              crossedDoor = *d;
              hasChangedRoom = true;
            }
          }
          break;
        }
      }
    }
    if (!insideDoorway && (p->x + shadowRadiusX) > rightWallX)
      p->x = rightWallX - shadowRadiusX;
  }

  // --- 3. BACK WALL COLLISION ---
  if ((p->y + shadowRadiusY) > r->length && !hasChangedRoom) {
    bool insideDoorway = false;
    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if (d->wall == WALL_BACK) {
        float centerMarginX = (d->width / 2.0f) - 2.0f;
        if (p->x >= (d->position - centerMarginX) &&
            p->x <= (d->position + centerMarginX)) {
          if (d->state == DOOR_CLOSED) {
            d->state = DOOR_OPENING;
            ApplyBounce(p, 0.0f, -BOUNCE_IMPULSE_FORCE);
            p->y = r->length - shadowRadiusY;
            insideDoorway = false;
          } else {
            insideDoorway = true;
            if (p->y > (r->length + DOOR_TRIGGER_THRESHOLD_BACK)) {
              crossedDoor = *d;
              hasChangedRoom = true;
            }
          }
          break;
        }
      }
    }
    if (!insideDoorway && (p->y + shadowRadiusY) > r->length)
      p->y = r->length - shadowRadiusY;
  }

  // --- 4. BLOCK SIDE DOORS IF PASSING FROM BELOW (moveY > 0) ---
  if (moveY > 0.0f) {
    Vector2 screenShadow = ProjectRoomPoint(p->x, p->y, r->width);
    screenShadow.x += game->cameraScroll.x;
    screenShadow.y += game->cameraScroll.y;

    for (int i = 0; i < r->doorCount; i++) {
      Door *d = &r->doors[i];
      if ((d->state == DOOR_OPEN || d->state == DOOR_OPENING || d->state == DOOR_CLOSING) &&
          (d->wall == WALL_LEFT || d->wall == WALL_RIGHT)) {
        float leafY = d->position + (d->width / 2.0f);
        const float shadowFloorY = SHADOW_RADIUS_Y / 1.55f;

        float wallX = (d->wall == WALL_LEFT) ? GetLeftWallX(leafY, r->width, r->length) : GetRightWallX(leafY, r->width, r->length);
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

        if (inLeafRangeScreen && prevY <= leafY + 2.0f) {
          if ((p->y + shadowFloorY) >= leafY) {
            p->y = leafY - shadowFloorY; // <--- MODIFICADO: Retiene la sombra por delante del vano
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
      if ((d->state == DOOR_OPEN || d->state == DOOR_OPENING || d->state == DOOR_CLOSING) &&
          (d->wall == WALL_LEFT || d->wall == WALL_RIGHT)) {
        float leafY = d->position + (d->width / 2.0f);
        const float shadowFloorY = SHADOW_RADIUS_Y / 1.55f;

        float wallX = (d->wall == WALL_LEFT) ? GetLeftWallX(leafY, r->width, r->length) : GetRightWallX(leafY, r->width, r->length);
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

        if (inLeafRangeScreen && prevY >= leafY - 2.0f) {
          if ((p->y - shadowFloorY) <= leafY) {
            if (d->state == DOOR_OPEN || d->state == DOOR_OPENING) {
              d->state = DOOR_CLOSING; // <--- MODIFICADO: Inicia el cierre de la puerta
            }
            p->y = leafY + shadowFloorY; // <--- MODIFICADO: Retiene a Maroc detras de la hoja mientras se cierra
          }
        }
      }
    }
  }

  // --- 6. BLOCK BACK DOORS IF PASSING FROM RIGHT (moveX < 0) ---
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
  screenShadowPos:;
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

  // --- 8. FRONT ROOM BOUNDARY COLLISION ---
  if ((p->y - shadowRadiusY) < 0.0f) {
    p->y = shadowRadiusY;
  }

  // --- 9. TRIGGER ROOM SWITCH ---
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

void UpdateServantPhysics(Game *game, float dt) {
  Servant *s = &game->servant;
  Room *r = &game->rooms[game->currentRoom];

  if (!s->active)
    return;

  float moveX = 0.0f;
  float moveY = 0.0f;

  // 1. Keyboard directional input
  if (IsKeyDown(KEY_RIGHT))
    moveX += 1.0f;
  if (IsKeyDown(KEY_LEFT))
    moveX -= 1.0f;
  if (IsKeyDown(KEY_UP))
    moveY -= 1.0f;
  if (IsKeyDown(KEY_DOWN))
    moveY += 1.0f;

  if (moveX != 0.0f || moveY != 0.0f) {
    float inputLength = sqrtf(moveX * moveX + moveY * moveY);
    moveX /= inputLength;
    moveY /= inputLength;

    // 2. Direct rectilinear displacement in screen pixels
    s->screenX += moveX * s->speed * dt;
    s->screenY += moveY * s->speed * dt;

    // 3. Absolute screen viewport padding (never leaves the display window)
    const float screenPadding = 8.0f;
    if (s->screenX < screenPadding)
      s->screenX = screenPadding;
    if (s->screenX > (SCREEN_WIDTH - screenPadding))
      s->screenX = SCREEN_WIDTH - screenPadding;
    if (s->screenY < screenPadding)
      s->screenY = screenPadding;
    if (s->screenY > (SCREEN_HEIGHT - screenPadding))
      s->screenY = SCREEN_HEIGHT - screenPadding;

    // 4. Vertical limits: front room floor threshold and top of the back wall
    Vector2 frontFloorCenter = ProjectRoomPoint(0.0f, 4.0f, r->width);
    float maxScreenBottom = frontFloorCenter.y + game->cameraScroll.y;
    if (s->screenY > maxScreenBottom) {
      s->screenY = maxScreenBottom;
    }

    Vector2 backWallBase =
        ProjectRoomPoint(0.0f, r->length, r->width);
    float minScreenTop =
        (backWallBase.y - r->wallHeight) + game->cameraScroll.y;
    if (s->screenY < minScreenTop) {
      s->screenY = minScreenTop;
    }

    // 5 Horizontal limits
    float leftFrontX = GetLeftWallX(0.0f, r->width, r->length);
    float rightFrontX = GetRightWallX(0.0f, r->width, r->length);
    Vector2 leftFrontCorner = ProjectRoomPoint(leftFrontX, 0.0f, r->width);
    Vector2 rightFrontCorner = ProjectRoomPoint(rightFrontX, 0.0f, r->width);

    float outermostLeftX = leftFrontCorner.x + game->cameraScroll.x;
    float outermostRightX = rightFrontCorner.x + game->cameraScroll.x;

    // Clamp the servant's screen position to these outermost vertical lines
    if (s->screenX < outermostLeftX) {
      s->screenX = outermostLeftX;
    } else if (s->screenX > outermostRightX) {
      s->screenX = outermostRightX;
    }

    float localY = -(s->screenY - game->cameraScroll.y) / (PROJECTION_Y_FACTOR * PROJECTION_SCALE);
    if (localY < 4.0f)
      localY = 4.0f;
    if (localY > r->length)
      localY = r->length;
    s->y = localY;

    float screenLeftLimit = GetLeftWallX(s->y, r->width, r->length) * PROJECTION_SCALE;
    float screenRightLimit = GetRightWallX(s->y, r->width, r->length) * PROJECTION_SCALE;

    float visibleRoomWidth = screenRightLimit - screenLeftLimit;
    float localScreenX = s->screenX - game->cameraScroll.x;

    if (visibleRoomWidth > 0.0f) {
      float factorX = (localScreenX - screenLeftLimit) / visibleRoomWidth;
      s->x = GetLeftWallX(s->y, r->width, r->length) + factorX * (GetRightWallX(s->y, r->width, r->length) - GetLeftWallX(s->y, r->width, r->length));
    }

    // Clamp logical coordinates to valid floor ground so dropping an item
    // always lands strictly inside the playable room scenario
    float minX = GetLeftWallX(s->y, r->width, r->length) + 4.0f;
    float maxX = GetRightWallX(s->y, r->width, r->length) - 4.0f;
    if (s->x < minX)
      s->x = minX;
    if (s->x > maxX)
      s->x = maxX;
  }
}

static Vector2 GetSideDoorHandleScreenPos(const Door *door, const Room *room,
                                          Vector2 cameraScroll) {
  float yFront = door->position - (door->width / 2.0f);
  float yBack = door->position + (door->width / 2.0f);
  float xWallFront = (door->wall == WALL_LEFT) ? GetLeftWallX(yFront, room->width, room->length) : GetRightWallX(yFront, room->width, room->length);
  float xWallBack = (door->wall == WALL_LEFT) ? GetLeftWallX(yBack, room->width, room->length) : GetRightWallX(yBack, room->width, room->length);

  Vector2 baseFront = ProjectRoomPoint(xWallFront, yFront, room->width);
  Vector2 baseBack = ProjectRoomPoint(xWallBack, yBack, room->width);
  baseFront.x += cameraScroll.x;
  baseFront.y += cameraScroll.y;
  baseBack.x += cameraScroll.x;
  baseBack.y += cameraScroll.y;

  float handleAltitude = door->height * 0.45f;
  Vector2 handlePos = {0.0f, 0.0f};

  if (door->state == DOOR_CLOSED || door->openProgress <= 0.0f) {
    // --- Closed door ---
    // Hinge is at baseBack. So handle is at the opposite side
    // : near the front (baseFront), 20% displaced to the center
    // centro.
    handlePos.x = baseFront.x + (baseBack.x - baseFront.x) * 0.20f;
    handlePos.y =
        (baseFront.y + (baseBack.y - baseFront.y) * 0.20f) - handleAltitude;
  } else {
    // --- Opne/Opening Door ---
    // Door leaf is anchores at baseBack. Door handle is on the free edge
    // (edgeBottom)
    Vector2 hingeBottom = baseBack;

    Vector2 doorwayVector = {baseFront.x - baseBack.x,
                             baseFront.y - baseBack.y};
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

    // Outer free edge of the wooden leaf
    Vector2 edgeBottom;
    edgeBottom.x = hingeBottom.x + (wallDir.x * leafLength * cosf(angle)) +
                   (inwardX * leafLength * sinf(angle));
    edgeBottom.y = hingeBottom.y + (wallDir.y * leafLength * cosf(angle));

    // The knob is located right at that free moving end (at 90% of the
    // distance from the hinge).
    const float handleRatio = 0.90f;
    handlePos.x = hingeBottom.x + (edgeBottom.x - hingeBottom.x) * handleRatio;
    handlePos.y =
        (hingeBottom.y + (edgeBottom.y - hingeBottom.y) * handleRatio) -
        handleAltitude;
  }

  return handlePos;
}

void UpdateServantDoorInteraction(Game *game) {
  Servant *s = &game->servant;
  Room *r = &game->rooms[game->currentRoom];

  if (!s->active)
    return;

  Vector2 servantCenter = {s->screenX, s->screenY};
  const float triggerRadius = 18.0f;
  const float triggerRadiusSq = triggerRadius * triggerRadius;

  for (int i = 0; i < r->doorCount; i++) {
    Door *d = &r->doors[i];

    // Free exits without wooden leaves are ignored
    if (d->state == DOOR_FREE_EXIT)
      continue;

    Vector2 handlePos = {0.0f, 0.0f};

    if (d->wall == WALL_BACK) {
      // Back door handle logic
      Vector2 doorBase = ProjectRoomPoint(d->position, r->length, r->width);
      doorBase.x += game->cameraScroll.x;
      doorBase.y += game->cameraScroll.y;

      if (d->state == DOOR_CLOSED || d->openProgress <= 0.0f) {
        handlePos.x = doorBase.x + (d->width * 0.25f);
        handlePos.y = doorBase.y - (d->height * 0.45f);
      } else {
        Vector2 hingeBottom = {doorBase.x - d->width / 2.0f, doorBase.y};
        float leafLength = d->width * 0.90f;
        float angle = d->openProgress * ((PI / 2.0f) * 0.88f);
        float dx =
            leafLength * cosf(angle) - (leafLength * 0.55f) * sinf(angle);
        float dy = (leafLength * 0.38f) * sinf(angle);
        Vector2 edgeBottom = {hingeBottom.x + dx, hingeBottom.y + dy};

        handlePos.x = hingeBottom.x + (edgeBottom.x - hingeBottom.x) * 0.85f;
        handlePos.y = (hingeBottom.y + (edgeBottom.y - hingeBottom.y) * 0.85f) -
                      (d->height * 0.45f);
      }
    } else {
      // Lateral doors (WALL_LEFT & WALL_RIGHT)
      handlePos = GetSideDoorHandleScreenPos(d, r, game->cameraScroll);
    }

    // Distance check between servant and handle
    float dx = servantCenter.x - handlePos.x;
    float dy = servantCenter.y - handlePos.y;
    float distSq = dx * dx + dy * dy;

    if (distSq <= triggerRadiusSq) {
      // Only toggle if the door is fully settled in its state
      if (d->state == DOOR_CLOSED && d->openProgress <= 0.05f) {
        d->state = DOOR_OPENING;
      } else if (d->state == DOOR_OPEN && d->openProgress >= 0.95f) {
        d->state = DOOR_CLOSING;
      }
    }
  }
}

// =============================================================================
// MAROC ITEM PICKUP & INVENTORY FIFO SYSTEM (src/physics.c)
// =============================================================================
void UpdateItemPickups(Game *game) {
  if (game->controlMode != CONTROL_MAROC)
    return;

  Player *p = &game->player;
  Room *r = &game->rooms[game->currentRoom];

  const float pickupRadius = 18.0f;
  const float pickupRadiusSq = pickupRadius * pickupRadius;

  bool standingOnLockoutItem = false;

  for (int i = 0; i < r->itemCount; i++) {
    Item *item = &r->items[i];

    // Ignore inactive objects or objects hanging on the wall (these are
    // collected by the servant)
    if (!item->active || item->isWallItem)
      continue;

    float dx = p->x - item->x;
    float dy = p->y - item->y;
    float distSq = dx * dx + dy * dy;

    if (distSq <= pickupRadiusSq) {
      if (item->id == p->pickupLockoutItemId) {
        standingOnLockoutItem = true;
        continue;
      }

      if (p->inventoryCount < MAX_INVENTORY_ITEMS) {
        // Add if there is space (<3)
        p->inventory[p->inventoryCount] = *item;
        p->inventoryCount++;
        item->active = false; // Remove from the  room floor
        p->pickupLockoutItemId = 0;
      } else {
        // INVENTORY FULL (3/3): FIFO SYSTEM (First In, First Out)
        Item droppedItem = p->inventory[0];
        droppedItem.x = item->x;
        droppedItem.y = item->y;
        droppedItem.isWallItem = false;
        droppedItem.active = true;

        p->inventory[0] = p->inventory[1];
        p->inventory[1] = p->inventory[2];

        p->inventory[2] = *item;

        *item = droppedItem;

        p->pickupLockoutItemId = droppedItem.id;
        standingOnLockoutItem = true;
      }
    }
  }
  if (!standingOnLockoutItem) {
    p->pickupLockoutItemId = 0;
  }
}
