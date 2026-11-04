#include "render.h"
#include "projection.h"
#include <math.h>

void DrawFloorGrid(const Room *room, Vector2 scroll) {
    for (float x = 0; x <= room->width; x += TILE_SIZE) {
        Vector2 p1 = ProjectRoomPoint(x, 0.0f, room->width);
        Vector2 p2 = ProjectRoomPoint(x, room->length, room->width);
        DrawLine(p1.x + scroll.x, p1.y + scroll.y, p2.x + scroll.x, p2.y + scroll.y, DARKGRAY);
    }

    for (float y = 0; y <= room->length; y += TILE_SIZE) {
        Vector2 p1 = ProjectRoomPoint(0.0f, y, room->width);
        Vector2 p2 = ProjectRoomPoint(room->width, y, room->width);
        DrawLine(p1.x + scroll.x, p1.y + scroll.y, p2.x + scroll.x, p2.y + scroll.y, DARKGRAY);
    }
}

void DrawWalls(const Room *room, Vector2 scroll) {
    Vector2 leftBottom  = ProjectRoomPoint(0.0f, 0.0f, room->width);
    Vector2 leftTop     = ProjectRoomPoint(0.0f, room->length, room->width);
    Vector2 rightTop    = ProjectRoomPoint(room->width, room->length, room->width);
    Vector2 rightBottom = ProjectRoomPoint(room->width, 0.0f, room->width);

    leftBottom.x  += scroll.x; leftBottom.y  += scroll.y;
    leftTop.x     += scroll.x; leftTop.y     += scroll.y;
    rightTop.x    += scroll.x; rightTop.y    += scroll.y;
    rightBottom.x += scroll.x; rightBottom.y += scroll.y;

    // Left wall
    DrawLineV(leftBottom, leftTop, room->wallColor);
    DrawLineV((Vector2){ leftBottom.x, leftBottom.y - room->wallHeight },
              (Vector2){ leftTop.x, leftTop.y - room->wallHeight }, room->wallColor);
    DrawLineV(leftBottom, (Vector2){ leftBottom.x, leftBottom.y - room->wallHeight }, room->wallColor);
    DrawLineV(leftTop, (Vector2){ leftTop.x, leftTop.y - room->wallHeight }, room->wallColor);

    // Back wall
    DrawLineV(leftTop, rightTop, room->wallColor);
    DrawLineV((Vector2){ leftTop.x, leftTop.y - room->wallHeight },
              (Vector2){ rightTop.x, rightTop.y - room->wallHeight }, room->wallColor);
    DrawLineV(rightTop, (Vector2){ rightTop.x, rightTop.y - room->wallHeight }, room->wallColor);

    // Right wall
    DrawLineV(rightTop, rightBottom, room->wallColor);
    DrawLineV((Vector2){ rightBottom.x, rightBottom.y - room->wallHeight },
              (Vector2){ rightTop.x, rightTop.y - room->wallHeight }, room->wallColor);
    DrawLineV(rightBottom, (Vector2){ rightBottom.x, rightBottom.y - room->wallHeight }, room->wallColor);
}

static void DrawBackDoor(const Door *door, const Room *room, Vector2 scroll) {
    Vector2 doorBase = ProjectRoomPoint(door->position, room->length, room->width);
    doorBase.x += scroll.x;
    doorBase.y += scroll.y;

    if (door->state == DOOR_FREE_EXIT) {
        DrawRectangle(doorBase.x - door->width / 2.0f, doorBase.y - door->height, door->width, door->height, BLACK);
        DrawRectangleLinesEx((Rectangle){ doorBase.x - door->width / 2.0f, doorBase.y - door->height, door->width, door->height }, 2.0f, door->color);
        return;
    }

    Color fillColor = (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) ? DARKGRAY : BLACK;
    DrawRectangle(doorBase.x - door->width / 2.0f, doorBase.y - door->height, door->width, door->height, fillColor);
    DrawRectangleLinesEx((Rectangle){ doorBase.x - door->width / 2.0f, doorBase.y - door->height, door->width, door->height }, 2.0f, door->color);

    if (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) {
        DrawText("X", doorBase.x - 5, doorBase.y - door->height / 2.0f - 5, 12, RED);
    } else if (door->openProgress > 0.0f) {
        Vector2 hingeBottom = { doorBase.x - door->width / 2.0f, doorBase.y };
        Vector2 hingeTop    = { hingeBottom.x, hingeBottom.y - door->height };

        float leafLength = door->width * 0.85f; // We can adjust final size here
        // Opening angle
        // 0.0 rad = CLOSED (leaf extended along right wall)
        // MAX_ANGLE rad = OPEN (leaf rotated X degrees toward the player / the room's interior)
        const float MAX_ANGLE = (PI / 2.0f) * 0.78f; // We can adjust final angle here
        float angle = door->openProgress * MAX_ANGLE;

         // Continuous rotation on the ground plane:
        // cos(angle) = 1 when closed (extension to the right), 0 when open
        // sin(angle) = 0 when closed, 1 when open (projection toward the front along the isometric Y-axis)
        // The X-component tilts toward the left as it opens, in order to align with the visual vanishing point          
        float dx = leafLength * cosf(angle) - (leafLength * 0.75f) * sinf(angle);
        float dy = (leafLength * 0.40f) * sinf(angle); // Descent in perspective toward the ground

        Vector2 edgeBottom = { hingeBottom.x + dx, hingeBottom.y + dy };
        Vector2 edgeTop    = { edgeBottom.x, edgeBottom.y - door->height };

        Color doorLeafColor = { 25, 25, 25, 255 };
        DrawTriangle(hingeBottom, hingeTop, edgeBottom, doorLeafColor);
        DrawTriangle(hingeTop, edgeTop, edgeBottom, doorLeafColor);

        DrawLineEx(hingeBottom, edgeBottom, 2.5f, door->color);
        DrawLineEx(edgeBottom, edgeTop, 2.5f, door->color);
        DrawLineEx(edgeTop, hingeTop, 2.5f, door->color);
    }
}

static void DrawSideDoor(const Door *door, const Room *room, Vector2 scroll) {
    float yFront = door->position - (door->width / 2.0f);
    float yBack  = door->position + (door->width / 2.0f);
    float xWall  = (door->wall == WALL_LEFT) ? 1.0f : (room->width - 1.0f);

    Vector2 baseFront = ProjectRoomPoint(xWall, yFront, room->width);
    Vector2 baseBack  = ProjectRoomPoint(xWall, yBack,  room->width);
    baseFront.x += scroll.x; baseFront.y += scroll.y;
    baseBack.x  += scroll.x; baseBack.y  += scroll.y;

    Vector2 roofFront = { baseFront.x, baseFront.y - door->height };
    Vector2 roofBack  = { baseBack.x,  baseBack.y  - door->height };

    if (door->state == DOOR_FREE_EXIT) {
        DrawTriangle(baseFront, baseBack, roofFront, BLACK);
        DrawTriangle(roofFront, baseBack, (Vector2){ baseBack.x, baseBack.y - door->height }, BLACK);
        DrawLineEx(baseFront, roofFront, 2.0f, door->color);
        DrawLineEx(roofFront, (Vector2){ baseBack.x, baseBack.y - door->height }, 2.0f, door->color);
        DrawLineEx((Vector2){ baseBack.x, baseBack.y - door->height }, baseBack, 2.0f, door->color);
        return;
    }

    Color fillColor = (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) ? DARKGRAY : BLACK;

    if (door->wall == WALL_LEFT) {
        DrawTriangle(baseFront, baseBack, roofFront, fillColor);
        DrawTriangle(roofFront, baseBack, roofBack,  fillColor);
    } else {
        DrawTriangle(baseFront, roofFront, baseBack, fillColor);
        DrawTriangle(roofFront, roofBack,  baseBack, fillColor);
    }

    DrawLineEx(baseFront, roofFront, 2.0f, door->color);
    DrawLineEx(roofFront, roofBack,  2.0f, door->color);
    DrawLineEx(roofBack,  baseBack,  2.0f, door->color);
    DrawLineEx(baseBack,  baseFront, 2.0f, door->color);

    if (door->state == DOOR_CLOSED && door->openProgress <= 0.0f) {
        float centerX = (baseFront.x + baseBack.x) / 2.0f;
        float centerY = (baseFront.y + baseBack.y) / 2.0f - door->height / 2.0f;
        DrawText("X", centerX - 4, centerY - 5, 12, RED);
    } else if (door->openProgress > 0.0f) {
        Vector2 hingeBottom = baseBack;
        Vector2 hingeTop    = roofBack;

        Vector2 doorwayVector = { baseFront.x - baseBack.x, baseFront.y - baseBack.y };
        float doorwayPixelSpan = sqrtf(doorwayVector.x * doorwayVector.x + doorwayVector.y * doorwayVector.y);
        float leafLength = doorwayPixelSpan * 1.1f; // NHR: Put a bigger number (now is 110%) for a larger open door

        Vector2 wallDir = { 0.0f, 0.0f };
        if (doorwayPixelSpan > 0.0f) {
            wallDir.x = doorwayVector.x / doorwayPixelSpan;
            wallDir.y = doorwayVector.y / doorwayPixelSpan;
        }

        // Ángulo de rotación de 0 a PI/2
        float angle = door->openProgress * (PI / 2.0f);

        // Vector normal hacia el interior de la sala
        float inwardX = (door->wall == WALL_LEFT) ? 1.0f : -1.0f;

        Vector2 edgeBottom;
        edgeBottom.x = hingeBottom.x + (wallDir.x * leafLength * cosf(angle)) + (inwardX * leafLength * sinf(angle));
        edgeBottom.y = hingeBottom.y + (wallDir.y * leafLength * cosf(angle));

        Vector2 edgeTop = { edgeBottom.x, edgeBottom.y - door->height };

        Color doorLeafColor = { 25, 25, 25, 255 };

        if (door->wall == WALL_LEFT) {
            DrawTriangle(hingeBottom, edgeBottom, hingeTop, doorLeafColor);
            DrawTriangle(hingeTop,    edgeBottom, edgeTop,  doorLeafColor);
        } else {
            DrawTriangle(hingeBottom, hingeTop, edgeBottom, doorLeafColor);
            DrawTriangle(hingeTop,    edgeTop,  edgeBottom, doorLeafColor);
        }

        DrawLineEx(hingeBottom, edgeBottom, 2.5f, door->color);
        DrawLineEx(edgeBottom, edgeTop,    2.5f, door->color);
        DrawLineEx(edgeTop,    hingeTop,   2.5f, door->color);
    }
}

void DrawDoors(const Room *room, Vector2 scroll) {
    for (int i = 0; i < room->doorCount; i++) {
        const Door *door = &room->doors[i];
        if (door->wall == WALL_BACK) {
            DrawBackDoor(door, room, scroll);
        } else {
            DrawSideDoor(door, room, scroll);
        }
    }
}

void DrawPlayer(const Game *game) {
    Vector2 shadowPos = {
        game->projectedPlayer.x + game->cameraScroll.x,
        game->projectedPlayer.y + game->cameraScroll.y
    };

    // Semi-transparent floor shadow oval
    DrawEllipse(shadowPos.x, shadowPos.y, SHADOW_RADIUS_X, SHADOW_RADIUS_Y, (Color){ 255, 255, 255, 85 });

    // Levitating character cube floating above the shadow
    float totalAltitude = game->player.z + game->player.floatingOffset;
    DrawRectangle(
        shadowPos.x - PLAYER_WIDTH / 2.0f,
        shadowPos.y - PLAYER_HEIGHT - totalAltitude,
        PLAYER_WIDTH,
        PLAYER_HEIGHT,
        WHITE
    );
}

void DrawHUD(const Game *game) {
    const Room *room = &game->rooms[game->currentRoom];
    DrawText(room->name, 20, 20, 20, YELLOW);
    DrawText(TextFormat("Room: %d | Doors: %d", game->currentRoom + 1, room->doorCount), 20, 48, 14, RAYWHITE);
}
