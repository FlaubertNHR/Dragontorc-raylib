#include "projection.h"
#include "config.h"

Vector2 ProjectRoomPoint(float x, float y, float roomWidth) {
    Vector2 screenPos;

    float factorY = y / FIXED_VISUAL_DEPTH;
    if (factorY < PROJECTION_MIN_FACTOR_Y) factorY = PROJECTION_MIN_FACTOR_Y; // Frontal limit
    if (factorY > 1.0f) factorY = 1.0f; // Back wall Limit

    float leftFrontX = PROJECTION_LEFT_FRONT_X;
    float leftBackX  = PROJECTION_LEFT_BACK_X;
    float leftScreenLimit = leftFrontX + (leftBackX - leftFrontX) * factorY;

    float rightFrontX = roomWidth - PROJECTION_LEFT_FRONT_X;
    float rightBackX  = roomWidth - PROJECTION_LEFT_BACK_X;
    float rightScreenLimit = rightFrontX + (rightBackX - rightFrontX) * factorY;

    float factorX = (roomWidth > 0.0f) ? (x / roomWidth) : 0.0f;

    screenPos.x = leftScreenLimit + (rightScreenLimit - leftScreenLimit) * factorX;
    screenPos.y = -(y * PROJECTION_Y_FACTOR);

    screenPos.x *= PROJECTION_SCALE;
    screenPos.y *= PROJECTION_SCALE;

    return screenPos;
}
