#include "physics.h"
#include "config.h"
#include "projection.h"
#include <math.h>

void ApplyBounce(Player *player, float dirX, float dirY) {
  float impactSpeed = player->moveRamp;
  if (impactSpeed < 0.15f) {
    player->moveRamp = 0.0f;
    return;
  }
  player->lastDirX = dirX;
  player->lastDirY = dirY;
  // Momentum of the rebound begins in proportion to the speed at which you collided
  player->moveRamp = impactSpeed;
  // Control-lockout duration scales in proportion to the shock intensity
  player->bounceLockoutTimer = BOUNCE_LOCKOUT_TIME * impactSpeed;
}

void UpdatePlayerPhysics(Game *game, float *outMoveX, float *outMoveY,
                         float dt) {
  Player *p = &game->player;

  float moveX = 0.0f;
  float moveY = 0.0f;

  float pixelsPerLogicalUnit = PROJECTION_SCALE;

  // 1. Reading keyboard input
  if (p->bounceLockoutTimer > 0.0f) {
    p->bounceLockoutTimer -= dt;
    // Durante el lockout no leemos teclado para dar prioridad al inicio del retroceso
  } else {
    if (IsKeyDown(KEY_RIGHT)) moveX += 1.0f;
    if (IsKeyDown(KEY_LEFT)) moveX -= 1.0f;
    if (IsKeyDown(KEY_UP)) moveY += 1.0f;
    if (IsKeyDown(KEY_DOWN)) moveY -= 1.0f;
  }

  bool isMovingInput = (moveX != 0.0f || moveY != 0.0f);
  float dirX = 0.0f;
  float dirY = 0.0f;

  // 2. Computing Acceleration Ramp (Takeoff / Landing)
  if (isMovingInput) {
    if (moveX != 0.0f && moveY != 0.0f) {
      moveX *= DIAGONAL_FACTOR_X;
    }
    float length = sqrtf(moveX * moveX + moveY * moveY);
    float inputDirX = moveX / length;
    float inputDirY = moveY / length;    

    // Detect direction reversal on either axis while player has levitation momentum
    bool isReversingX = (p->lastDirX * inputDirX < 0.0f);
    bool isReversingY = (p->lastDirY * inputDirY < 0.0f);

    if (p->moveRamp > 0.0f && (isReversingX || isReversingY)) {
      // Smooth inertia deceleration and landing in the previous direction before turning
      p->moveRamp -= dt / DECELERATION_TIME;
      if (p->moveRamp < 0.0f) p->moveRamp = 0.0f;
      dirX = p->lastDirX;
      dirY = p->lastDirY;
    } else {
      // Normal acceleration / transition into new direction
      p->lastDirX = inputDirX;
      p->lastDirY = inputDirY;
      dirX = inputDirX;
      dirY = inputDirY;

      p->moveRamp += dt / ACCELERATION_TIME;
      if (p->moveRamp > 1.0f) p->moveRamp = 1.0f;
    }
 
  } else {
    p->moveRamp -= dt / DECELERATION_TIME;
    if (p->moveRamp < 0.0f) p->moveRamp = 0.0f;

    // We use the last direction to maintain momentum when keys are released
    dirX = p->lastDirX;
    dirY = p->lastDirY;
  }

  // 3. Harmonic elevation modulated by the ramp (gradually decreases to 0 during deceleration)
  p->levitationTimer += dt * LEVITATION_SPEED;
  float targetAltitude = PLAYER_BASE_Z + sinf(p->levitationTimer) * LEVITATION_AMPLITUDE;
  p->floatingOffset = targetAltitude * p->moveRamp;

  // 4. Inertial coasting while the ramp value is greater than 0
  if (p->moveRamp > 0.0f && (dirX != 0.0f || dirY != 0.0f)) {
    float effectiveSpeed = p->speed * p->moveRamp;
    float stepX = (dirX * (effectiveSpeed * SPEED_MULTIPLIER_X) * dt) / pixelsPerLogicalUnit;
    float stepY = dirY * effectiveSpeed * dt;

    p->x += stepX;
    p->y += stepY;
  }
  // Return the vector containing the inertia component for collisions with doors
  *outMoveX = (p->moveRamp > 0.0f) ? p->lastDirX * p->moveRamp : 0.0f;
  *outMoveY = (p->moveRamp > 0.0f) ? p->lastDirY * p->moveRamp : 0.0f;
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
        game->player.x = leftWall + DOOR_TRANSITION_MARGIN_X; // A step inward
        game->player.y = dest->position;
      } else if (dest->wall == WALL_RIGHT) {
        float rightWall = GetRightWallX(dest->position, nextRoom->width, nextRoom->length);
        game->player.x = rightWall - DOOR_TRANSITION_MARGIN_X;
        game->player.y = dest->position;
      } else if (dest->wall == WALL_BACK) {
        game->player.x = dest->position;
        game->player.y = nextRoom->length - DOOR_TRANSITION_MARGIN_Y;
      }
      break;
    }
  }
#if RESET_MOTION_ON_ROOM_TRANSITION
  // Reset movement state when entering new room
  game->player.moveRamp = 0.0f;
  game->player.floatingOffset = 0.0f;
  game->player.lastDirX = 0.0f;
  game->player.lastDirY = 0.0f;
#endif

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
  if ((p->x - shadowRadiusX) < leftWallX) {
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
            p->x = leftWallX + shadowRadiusX;
            ApplyBounce(p, -p->lastDirX, -p->lastDirY * 0.5);
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
    if (!insideDoorway && (p->x - shadowRadiusX) < leftWallX) {
      p->x = leftWallX + shadowRadiusX;
      if (p->bounceLockoutTimer <= 0.0f) {
        ApplyBounce(p, -p->lastDirX, -p->lastDirY * 0.5);
      }
    }
  }

  // --- 2. RIGHT WALL COLLISION ---
  if (((p->x + shadowRadiusX) > rightWallX) && !hasChangedRoom) {
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
            p->x = rightWallX - shadowRadiusX;
            ApplyBounce(p, -p->lastDirX, -p->lastDirY * 0.5);
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
    if (!insideDoorway && (p->x + shadowRadiusX) > rightWallX) {
      p->x = rightWallX - shadowRadiusX;
      if (p->bounceLockoutTimer <= 0.0f) {
        ApplyBounce(p, -p->lastDirX, -p->lastDirY * 0.5);
      }
    }
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
            p->y = r->length - shadowRadiusY;
            ApplyBounce(p, -p->lastDirX, -p->lastDirY);
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
    if (!insideDoorway && (p->y + shadowRadiusY) > r->length) {
      p->y = r->length - shadowRadiusY;
      if (p->bounceLockoutTimer <= 0.0f) {
        ApplyBounce(p, p->lastDirX * 0.5, -p->lastDirY);
      }
    }
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
              d->state = DOOR_CLOSING;
            }
            p->y = leafY + shadowFloorY;
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
    if (p->bounceLockoutTimer <= 0.0f) {
      ApplyBounce(p, p->lastDirX * 0.5, -p->lastDirY); // Rebote opuesto a la inercia de choque
    }
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

  if (s->marocInteractionLockoutTimer > 0.0f) {
    s->marocInteractionLockoutTimer -= dt;
  }

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

    // Outer radius of the servant sprite circle (8.5f px in DrawServant)
    const float servantRadius = 8.5f;

    // 3. Vertial screen limits
    float minScreenTop = VIEWPORT_Y + servantRadius;
    if (s->screenY < minScreenTop) {
      s->screenY = minScreenTop;
    }

    // Bottom limit / Viewport exit check:
    // The servant moves freely downwards past the bottom of the room floor.
    // When its top edge goes past the bottom boundary of the viewport (VIEWPORT_Y + VIEWPORT_HEIGHT),
    // it is completely clipped by ScissorMode (hidden behind the black bottom viewport strip).
    // At this point, deactivate the servant and return control to Maroc.
    float viewportBottom = VIEWPORT_Y + VIEWPORT_HEIGHT;
    if ((s->screenY - servantRadius) >= viewportBottom) {
      if (s->carriedItemId != 0) {
        StartServantItemDrop(game);
      }
      game->servant.active = false;
      game->controlMode = CONTROL_MAROC;
      return;
    }

    // 4. Horizontal limits: do not encroach upon the left or right side margins
    float minScreenLeft = VIEWPORT_X + servantRadius;
    float maxScreenRight = (VIEWPORT_X + VIEWPORT_WIDTH) - servantRadius;

    if (s->screenX < minScreenLeft) {
      s->screenX = minScreenLeft;
    }
    if (s->screenX > maxScreenRight) {
      s->screenX = maxScreenRight;
    }

    // 5. Logical room coordinates mapping (x, y) for picking up / dropping items
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

  if (!s->active || s->carriedItemId != 0)
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

// =============================================================================
// SERVANT ITEM PICKUP & MAROC INVENTORY INTERACTION (src/physics.c)
// =============================================================================
void UpdateServantItemInteraction(Game *game, float dt) {
  (void)dt;
  Servant *s = &game->servant;
  Player *p = &game->player;
  Room *r = &game->rooms[game->currentRoom];

  if (!s->active)
    return;

  // 1. PRECISE POSITION OF MAROC’S TORSO / BODY ON SCREEN
  // (Taking into account Maroc’s height and his current levitation)
  float shadowScreenX = game->projectedPlayer.x + game->cameraScroll.x;
  float shadowScreenY = game->projectedPlayer.y + game->cameraScroll.y;
  float currentAltitude = p->z + p->floatingOffset;

  // Visual center of Maroc’s body/torso (above the shadow on the ground)
  float marocBodyCenterX = shadowScreenX;
  float marocBodyCenterY = shadowScreenY - currentAltitude - (PLAYER_HEIGHT * 0.55f);

  // 2. MAROC BODY HITBOX
  // Tightly adjusted so that when the Servant is summoned (appearing above the player's head),
  // it does not touch the player's body until the player intentionally moves it downward.
  float bodyWidth = 20.0f;  // Width of the torso contact area
  float bodyHeight = 20.0f; // Height of the torso contact area

  float dx = s->screenX - marocBodyCenterX;
  float dy = s->screenY - marocBodyCenterY;

  bool isTouchingMaroc = (fabsf(dx) <= (bodyWidth * 0.5f)) &&
                         (fabsf(dy) <= (bodyHeight * 0.5f));

  // ID blocking to prevent repeated exchanges without leaving the contact zone
  static int servantLockoutItemId = 0;

  // If the Servant leaves Maroc’s body, the lock resets instantly.
  if (!isTouchingMaroc) {
    servantLockoutItemId = 0;
  }

  // ---------------------------------------------------------------------------
  // 3. EXCHANGE WITH MAROC (FIFO SYSTEM WITH ID-BASED LOCKING)
  // ---------------------------------------------------------------------------
  if (isTouchingMaroc) {
    bool isLockedOut = false;

    if (s->carriedItemId != 0 && s->carriedItemId == servantLockoutItemId) {
      isLockedOut = true;
    } else if (s->carriedItemId == 0 && servantLockoutItemId != 0) {
      isLockedOut = true;
    }

    if (!isLockedOut) {
      // CASE A: Servant with EMPTY hands -> Extracts the OLDEST object (slot 0)
      if (s->carriedItemId == 0) {
        if (p->inventoryCount > 0) {
          Item oldestItem = p->inventory[0];

          // Shift the remaining Maroc objects to the left in the HUD
          for (int k = 0; k < p->inventoryCount - 1; k++) {
            p->inventory[k] = p->inventory[k + 1];
          }
          p->inventory[p->inventoryCount - 1] = (Item){0};
          p->inventoryCount--;

          // The Servant receives the extracted object
          s->carriedItem = oldestItem;
          s->carriedItemId = oldestItem.id;
          servantLockoutItemId = oldestItem.id; // Block this ID while it remains in contact
          return;
        }
      }
      // CASE B: Servant IS CARRYING AN OBJECT -> Hands it over to Maroc or performs a FIFO rotation
      else {
        int givenId = s->carriedItemId;

        if (p->inventoryCount < MAX_INVENTORY_ITEMS) {
          // Maroc has space (< 3): Place the object in the rightmost empty slot
          p->inventory[p->inventoryCount] = s->carriedItem;
          p->inventoryCount++;

          s->carriedItemId = 0;
          s->carriedItem = (Item){0};
          servantLockoutItemId = givenId; // Blocks the provided ID as long as it remains in contact
          return;
        } else {
          // Maroc Inventory FULL (3/3): FIFO ROTATION WITH SHIFTING
          Item oldestItem = p->inventory[0];

          // Shifts cells to the left
          p->inventory[0] = p->inventory[1];
          p->inventory[1] = p->inventory[2];

          // The new object enters the cell on the right (position 2)
          p->inventory[2] = s->carriedItem;

          // The Servant keeps the oldest ejected object
          s->carriedItem = oldestItem;
          s->carriedItemId = oldestItem.id;
          servantLockoutItemId = oldestItem.id; // Locks the received object
          return;
        }
      }
    }
  }

  // ---------------------------------------------------------------------------
  // 4. COLLECTION OF ITEMS IN THE ROOM (Floor / Walls)
  // ---------------------------------------------------------------------------
  if (s->carriedItemId == 0 && !isTouchingMaroc) {
    const float servantPickupRadius = 24.0f;
    const float servantPickupRadiusSq = servantPickupRadius * servantPickupRadius;
    Vector2 servantScreen = {s->screenX, s->screenY};

    for (int i = 0; i < r->itemCount; i++) {
      Item *item = &r->items[i];
      if (!item->active)
        continue;

      Vector2 itemScreen;
      if (item->isWallItem) {
        if (item->wall == WALL_BACK) {
          itemScreen = ProjectRoomPoint(item->x, r->length, r->width);
        } else if (item->wall == WALL_LEFT) {
          float wallX = GetLeftWallX(item->y, r->width, r->length);
          itemScreen = ProjectRoomPoint(wallX, item->y, r->width);
        } else {
          float wallX = GetRightWallX(item->y, r->width, r->length);
          itemScreen = ProjectRoomPoint(wallX, item->y, r->width);
        }
        itemScreen.y -= item->wallHeightOffset;
      } else {
        itemScreen = ProjectRoomPoint(item->x, item->y, r->width);
      }
      itemScreen.x += game->cameraScroll.x;
      itemScreen.y += game->cameraScroll.y;

      float idx = servantScreen.x - itemScreen.x;
      float idy = servantScreen.y - itemScreen.y;
      if ((idx * idx + idy * idy) <= servantPickupRadiusSq) {
        s->carriedItem = *item;
        s->carriedItemId = item->id;
        item->active = false; // Object leaves the room
        break;
      }
    }
  }
}

void StartServantItemDrop(Game *game) {
  Servant *s = &game->servant;
  Room *r = &game->rooms[game->currentRoom];

  if (s->carriedItemId == 0)
    return;

  // 1. Search for a free slot for the falling item animation
  int slot = -1;
  for (int i = 0; i < MAX_FALLING_ITEMS; i++) {
    if (!game->fallingItems[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1)
    slot = 0;

  FallingItem *fi = &game->fallingItems[slot];

  // 2. Initial screen position matching DrawServant visual clamping (frontmost vertical line)
  const float marginPx = 8.0f;
  float minScreenX = (GetLeftWallX(0.0f, r->width, r->length) * PROJECTION_SCALE) + game->cameraScroll.x + marginPx;
  float maxScreenX = (GetRightWallX(0.0f, r->width, r->length) * PROJECTION_SCALE) + game->cameraScroll.x - marginPx;

  float startX = s->screenX;
  if (startX < minScreenX) startX = minScreenX;
  if (startX > maxScreenX) startX = maxScreenX;

  float startY = s->screenY + 14.0f; // Held right below servant's hands

  // 3. Exact logical X coordinate for a 100% pure vertical drop line on screen
  float rawLogicalX = (startX - game->cameraScroll.x) / PROJECTION_SCALE;
  float targetLogicalX = rawLogicalX;

  // 4. Initial target depth Y starting at servant's depth (s->y)
  float targetLogicalY = s->y;

  // 5. Geometric Y-depth calculation for side wall intersection
  const float floorMarginX = 6.0f; // Clearance margin inside the side wall
  float halfWidth = r->width * 0.5f;

  float leftWallAtY = GetLeftWallX(targetLogicalY, r->width, r->length) + floorMarginX;
  float rightWallAtY = GetRightWallX(targetLogicalY, r->width, r->length) - floorMarginX;

  if (rawLogicalX < leftWallAtY) {
    // Over left wall: calculate forward Y-depth where left wall meets rawLogicalX
    targetLogicalY = r->length - (-halfWidth + floorMarginX - rawLogicalX) / SIDE_WALL_INCLINATION_SLOPE;
  } else if (rawLogicalX > rightWallAtY) {
    // Over right wall: calculate forward Y-depth where right wall meets rawLogicalX
    targetLogicalY = r->length - (rawLogicalX + floorMarginX - halfWidth) / SIDE_WALL_INCLINATION_SLOPE;
  }

  // 6. Random Y-offset dispersion cnverted from screen pixels to logical Y
  // Random screen displacement between 15px and 65px downwards, uncompressed by PROJECTION_Y_FACTOR
  float randomScreenOffsetPx = (float)GetRandomValue(15, 65);
  float randomYOffsetLogical = randomScreenOffsetPx / (PROJECTION_Y_FACTOR * PROJECTION_SCALE);
  targetLogicalY -= randomYOffsetLogical;

  // Strict safety clamping for front & back wall boundaries.
  // frontMarginY = 10.0f guarantees targetLogicalY NEVER reaches or exceeds y = 0.0f (front wall line)
  const float backMarginY = 25.0f;
  const float frontMarginY = 10.0f;

  if (targetLogicalY < frontMarginY) {
    targetLogicalY = frontMarginY;
  }
  if (targetLogicalY > (r->length - backMarginY)) {
    targetLogicalY = r->length - backMarginY;
  }

  // 7. Calculate floor shadow Y at release time to compute initial height above floor
  Vector2 targetProj = ProjectRoomPoint(targetLogicalX, targetLogicalY, r->width);
  float targetScreenYCalculated = targetProj.y + game->cameraScroll.y;

  float initialHeight = targetScreenYCalculated - startY;
  if (initialHeight < 20.0f) {
    initialHeight = 20.0f;
  }

  // 8. Populate falling item structure (anchored to room world coordinates)
  fi->active = true;
  fi->item = s->carriedItem;
  fi->startY = initialHeight;          // Initial height above floor shadow (px)
  fi->currentScreenY = initialHeight;  // Current height remaining above floor shadow (px)
  fi->targetLogicalX = targetLogicalX; // Logical room X
  fi->targetLogicalY = targetLogicalY; // Logical room Y
  fi->velocityY = 60.0f;               // Initial fall velocity

  // Release item from servant
  s->carriedItemId = 0;
  s->carriedItem = (Item){0};
}

void UpdateFallingItems(Game *game, float dt) {
  Room *r = &game->rooms[game->currentRoom];

  for (int k = 0; k < MAX_FALLING_ITEMS; k++) {
    FallingItem *fi = &game->fallingItems[k];
    if (!fi->active)
      continue;

    // Gravity acceleration reducing height above floor shadow
    fi->velocityY += 450.0f * dt;
    fi->currentScreenY -= fi->velocityY * dt;

    // Landing check when height reaches zero (or below)
    if (fi->currentScreenY <= 0.0f) {
      fi->currentScreenY = 0.0f;

      // Small elastic rebound if impact velocity is significant
      const float bounceThresholdSpeed = 60.0f;
      const float restitution = 0.35f;

      if (fi->velocityY > bounceThresholdSpeed) {
        fi->velocityY = -fi->velocityY * restitution;
      } else {
        // Register item on room floor at (targetLogicalX, targetLogicalY)
        bool itemFoundInRoom = false;
        for (int i = 0; i < r->itemCount; i++) {
          if (r->items[i].id == fi->item.id) {
            r->items[i] = fi->item;
            r->items[i].active = true;
            r->items[i].isWallItem = false;
            r->items[i].x = fi->targetLogicalX;
            r->items[i].y = fi->targetLogicalY;
            itemFoundInRoom = true;
            break;
          }
        }

        if (!itemFoundInRoom) {
          bool slotFound = false;
          for (int i = 0; i < r->itemCount; i++) {
            if (!r->items[i].active) {
              r->items[i] = fi->item;
              r->items[i].active = true;
              r->items[i].isWallItem = false;
              r->items[i].x = fi->targetLogicalX;
              r->items[i].y = fi->targetLogicalY;
              slotFound = true;
              break;
            }
          }
          if (!slotFound && r->itemCount < MAX_ITEMS_PER_ROOM) {
            r->items[r->itemCount] = fi->item;
            r->items[r->itemCount].active = true;
            r->items[r->itemCount].isWallItem = false;
            r->items[r->itemCount].x = fi->targetLogicalX;
            r->items[r->itemCount].y = fi->targetLogicalY;
            r->itemCount++;
          }
        }

        fi->active = false;
      }
    }
  }
}
