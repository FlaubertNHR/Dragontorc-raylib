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
    moveY -= 1.0f; // Pure screen UP (-Y)
  if (IsKeyDown(KEY_DOWN))
    moveY += 1.0f; // Pure screen DOWN (+Y)

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
    Vector2 frontFloorCenter =
        ProjectRoomPoint(r->width * 0.5f, 4.0f, r->width);
    float maxScreenBottom = frontFloorCenter.y + game->cameraScroll.y;
    if (s->screenY > maxScreenBottom) {
      s->screenY = maxScreenBottom;
    }

    Vector2 backWallBase =
        ProjectRoomPoint(r->width * 0.5f, r->length, r->width);
    float minScreenTop =
        (backWallBase.y - r->wallHeight) + game->cameraScroll.y;
    if (s->screenY < minScreenTop) {
      s->screenY = minScreenTop;
    }

    // =====================================================================
    // 5. HORIZONTAL LIMITS: OUTERMOST VERTICAL WALL BOUNDARIES
    // =====================================================================
    // The outermost vertical edges of the room walls are given by the
    // front floor corners (x = 0.0f and x = r->width at y = 0.0f).
    Vector2 leftFrontCorner = ProjectRoomPoint(0.0f, 0.0f, r->width);
    Vector2 rightFrontCorner = ProjectRoomPoint(r->width, 0.0f, r->width);

    float outermostLeftX = leftFrontCorner.x + game->cameraScroll.x;
    float outermostRightX = rightFrontCorner.x + game->cameraScroll.x;

    // Clamp the servant's screen position to these outermost vertical lines
    if (s->screenX < outermostLeftX) {
      s->screenX = outermostLeftX;
    } else if (s->screenX > outermostRightX) {
      s->screenX = outermostRightX;
    }

    // =====================================================================
    // 6. DERIVE LOGICAL COORDINATES (s->x, s->y) FOR ITEM/DOOR INTERACTIONS
    // =====================================================================
    float localY = -(s->screenY - game->cameraScroll.y) / (0.45f * 1.3f);
    if (localY < 4.0f)
      localY = 4.0f;
    if (localY > r->length)
      localY = r->length;
    s->y = localY;

    // Calculate trapezoid horizontal bounds at current depth (s->y)
    float factorY = s->y / FIXED_VISUAL_DEPTH;
    if (factorY < 0.0f)
      factorY = 0.0f;
    if (factorY > 1.0f)
      factorY = 1.0f;

    float frontLeftX = -310.0f;
    float backLeftX = -130.0f;
    float screenLeftLimit =
        (frontLeftX + (backLeftX - frontLeftX) * factorY) * 1.3f;

    float frontRightX = r->width + 310.0f;
    float backRightX = r->width + 130.0f;
    float screenRightLimit =
        (frontRightX + (backRightX - frontRightX) * factorY) * 1.3f;

    float visibleRoomWidth = screenRightLimit - screenLeftLimit;
    float localScreenX = s->screenX - game->cameraScroll.x;

    if (visibleRoomWidth > 0.0f) {
      float factorX = (localScreenX - screenLeftLimit) / visibleRoomWidth;
      s->x = factorX * r->width;
    }

    // Clamp logical coordinates to valid floor ground so dropping an item
    // always lands strictly inside the playable room scenario
    if (s->x < 4.0f)
      s->x = 4.0f;
    if (s->x > (r->width - 4.0f))
      s->x = r->width - 4.0f;
  }
}

static Vector2 GetSideDoorHandleScreenPos(const Door *door, const Room *room,
                                          Vector2 cameraScroll) {
  float yFront = door->position - (door->width / 2.0f); // Latch / front frame
  float yBack = door->position + (door->width / 2.0f);  // Hinge / rear frame
  float xWall = (door->wall == WALL_LEFT) ? 1.0f : (room->width - 1.0f);

  Vector2 baseFront = ProjectRoomPoint(xWall, yFront, room->width);
  Vector2 baseBack = ProjectRoomPoint(xWall, yBack, room->width);
  baseFront.x += cameraScroll.x;
  baseFront.y += cameraScroll.y;
  baseBack.x += cameraScroll.x;
  baseBack.y += cameraScroll.y;

  float handleAltitude = door->height * 0.45f;
  Vector2 handlePos = {0.0f, 0.0f};

  if (door->state == DOOR_CLOSED || door->openProgress <= 0.0f) {
    // --- PUERTA CERRADA ---
    // La bisagra está en baseBack. Por tanto, el picaporte está en el lado
    // opuesto: junto al marco frontal (baseFront), desplazado un 20% hacia el
    // centro.
    handlePos.x = baseFront.x + (baseBack.x - baseFront.x) * 0.20f;
    handlePos.y =
        (baseFront.y + (baseBack.y - baseFront.y) * 0.20f) - handleAltitude;
  } else {
    // --- PUERTA ABIERTA / ABRIÉNDOSE ---
    // La hoja gira anclada en baseBack. El picaporte está en el borde libre
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

    // Borde libre exterior de la hoja de madera
    Vector2 edgeBottom;
    edgeBottom.x = hingeBottom.x + (wallDir.x * leafLength * cosf(angle)) +
                   (inwardX * leafLength * sinf(angle));
    edgeBottom.y = hingeBottom.y + (wallDir.y * leafLength * cosf(angle));

    // El pomo está situado justo en ese extremo libre móvil (al 90% de la
    // distancia desde la bisagra)
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
