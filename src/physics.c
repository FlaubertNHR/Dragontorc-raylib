#include "physics.h"
#include "projection.h"
#include <math.h>

void ApplyBounce(Player *player, float forceX, float forceY) {
    player->bounceForceX = forceX;
    player->bounceForceY = forceY;
    player->bounceLockoutTimer = BOUNCE_LOCKOUT_TIME;
}

void UpdatePlayerPhysics(Game *game, float *outMoveX, float *outMoveY, float dt) {
    Player *p = &game->player;
    Room *r = &game->rooms[game->currentRoom];

    float moveX = 0.0f;
    float moveY = 0.0f;

    // Levitation smooth floating oscillation (bobbing)
    p->levitationTimer += dt * 4.0f;
    p->floatingOffset = sinf(p->levitationTimer) * 3.5f;

    // Visual unit ratio to ensure identical screen movement speed across all room sizes
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
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) moveX += 1.0f;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) moveX -= 1.0f;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) moveY += 1.0f;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) moveY -= 1.0f;

        if (moveX != 0.0f || moveY != 0.0f) {
            if (moveX != 0.0f && moveY != 0.0f) {
                moveX *= DIAGONAL_FACTOR_X;
            }
            float length = sqrtf(moveX * moveX + moveY * moveY);
            moveX /= length;
            moveY /= length;

            float stepX = 0.0f;
            if (pixelsPerLogicalUnit > 0.0f) {
                stepX = (moveX * (p->speed * SPEED_MULTIPLIER_X) * dt) / pixelsPerLogicalUnit;
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
                game->player.x = 3.0f;
                game->player.y = dest->position;
            } else if (dest->wall == WALL_RIGHT) {
                game->player.x = nextRoom->width - 3.0f;
                game->player.y = dest->position;
            } else if (dest->wall == WALL_BACK) {
                game->player.x = dest->position;
                game->player.y = nextRoom->length - 12.0f;
            }
            break;
        }
    }

    game->currentRoom = nextRoomIndex;
}

void UpdateCollisions(Game *game, float prevX, float prevY, float moveX, float moveY) {
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
                if (p->y >= (d->position - centerMarginY) && p->y <= (d->position + centerMarginY)) {
                    if (d->state == DOOR_CLOSED) {
                        d->state = DOOR_OPEN;
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
        if (!insideDoorway && p->x < 0.0f) p->x = 0.0f;
    }

    // --- 2. RIGHT WALL COLLISION ---
    if (p->x > (r->width - 8.0f) && !hasChangedRoom) {
        bool insideDoorway = false;
        for (int i = 0; i < r->doorCount; i++) {
            Door *d = &r->doors[i];
            if (d->wall == WALL_RIGHT) {
                float centerMarginY = d->width / 4.0f;
                if (p->y >= (d->position - centerMarginY) && p->y <= (d->position + centerMarginY)) {
                    if (d->state == DOOR_CLOSED) {
                        d->state = DOOR_OPEN;
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
        if (!insideDoorway && p->x > r->width) p->x = r->width;
    }

    // --- 3. BACK WALL COLLISION ---
    if (p->y > r->length && !hasChangedRoom) {
        bool insideDoorway = false;
        for (int i = 0; i < r->doorCount; i++) {
            Door *d = &r->doors[i];
            if (d->wall == WALL_BACK) {
                float centerMarginX = d->width / 4.0f;
                if (p->x >= (d->position - centerMarginX) && p->x <= (d->position + centerMarginX)) {
                    if (d->state == DOOR_CLOSED) {
                        d->state = DOOR_OPEN;
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
        if (!insideDoorway) p->y = r->length;
    }

    // --- 4. CLOSE SIDE DOORS VIA VERTICAL PUSH (moveY < 0) ---
    if (moveY < 0.0f) {
        for (int i = 0; i < r->doorCount; i++) {
            Door *d = &r->doors[i];
            if (d->state == DOOR_OPEN) {
                float leafLength = d->height * 0.75f;
                if (d->wall == WALL_LEFT) {
                    if (p->x >= 0.0f && p->x <= leafLength) {
                        if (prevY >= d->position && p->y < d->position) {
                            d->state = DOOR_CLOSED;
                            p->y = d->position + 8.0f;
                        }
                    }
                } else if (d->wall == WALL_RIGHT) {
                    if (p->x >= (r->width - leafLength) && p->x <= r->width) {
                        if (prevY >= d->position && p->y < d->position) {
                            d->state = DOOR_CLOSED;
                            p->y = d->position + 8.0f;
                        }
                    }
                }
            }
        }
    }

    // --- 5. CLOSE BACK DOOR VIA HORIZONTAL PUSH (moveX > 0) ---
    if (moveX > 0.0f) {
        Vector2 projectedPos = ProjectRoomPoint(p->x, p->y, r->width);
        Vector2 screenShadowPos = { projectedPos.x + game->cameraScroll.x, projectedPos.y + game->cameraScroll.y };

        for (int i = 0; i < r->doorCount; i++) {
            Door *d = &r->doors[i];
            if (d->wall == WALL_BACK && d->state == DOOR_OPEN) {
                Vector2 doorBase = ProjectRoomPoint(d->position, r->length, r->width);
                doorBase.x += game->cameraScroll.x;
                doorBase.y += game->cameraScroll.y;

                Vector2 hingeBottom = { doorBase.x - d->width / 2.0f, doorBase.y };
                Vector2 edgeBottom  = { doorBase.x - d->width / 2.0f - d->width * 0.75f, doorBase.y + d->height * 0.25f };

                float leafCenterX = (hingeBottom.x + edgeBottom.x) / 2.0f;
                float leafCenterY = (hingeBottom.y + edgeBottom.y) / 2.0f;

                float distX = fabsf(screenShadowPos.x - leafCenterX);
                float distY = fabsf(screenShadowPos.y - leafCenterY);

                if (distX < 20.0f && distY < 12.0f) {
                    d->state = DOOR_CLOSED;
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
