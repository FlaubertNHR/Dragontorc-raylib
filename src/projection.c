#include "projection.h"
#include "config.h"

float GetLeftWallX(float y, float roomWidth) {
    (void)roomWidth;
    float factorY = y / FIXED_VISUAL_DEPTH;
    if (factorY < PROJECTION_MIN_FACTOR_Y) factorY = PROJECTION_MIN_FACTOR_Y;
    if (factorY > 1.0f) factorY = 1.0f;

    return PROJECTION_LEFT_FRONT_X + (PROJECTION_LEFT_BACK_X - PROJECTION_LEFT_FRONT_X) * factorY;
}

float GetRightWallX(float y, float roomWidth) {
    float leftX = GetLeftWallX(y, roomWidth);
    return roomWidth - leftX;
}

Vector2 ProjectRoomPoint(float x, float y, float roomWidth) {
    (void)roomWidth;
    Vector2 screenPos;
    screenPos.x = x * PROJECTION_SCALE;
    screenPos.y = -(y * PROJECTION_Y_FACTOR) * PROJECTION_SCALE;
    return screenPos;
}
