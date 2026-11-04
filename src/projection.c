#include "projection.h"
#include "config.h"

Vector2 ProjectRoomPoint(float x, float y, float roomWidth) {
    Vector2 screenPos;

    float factorY = y / FIXED_VISUAL_DEPTH;
    if (factorY < 0.0f) factorY = 0.0f;
    if (factorY > 1.0f) factorY = 1.0f;

    float leftFrontX = -310.0f;
    float leftBackX  = -130.0f;
    float leftScreenLimit = leftFrontX + (leftBackX - leftFrontX) * factorY;

    float rightFrontX = roomWidth + 310.0f;
    float rightBackX  = roomWidth + 130.0f;
    float rightScreenLimit = rightFrontX + (rightBackX - rightFrontX) * factorY;

    float factorX = (roomWidth > 0.0f) ? (x / roomWidth) : 0.0f;

    screenPos.x = leftScreenLimit + (rightScreenLimit - leftScreenLimit) * factorX;
    screenPos.y = -(y * 0.45f);

    screenPos.x *= 1.3f;
    screenPos.y *= 1.3f;

    return screenPos;
}
