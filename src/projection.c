#include "projection.h"
#include "config.h"

float GetLeftWallX(float y, float roomWidth, float roomLength) {
    // Distance from the actual back wall of the room (y = roomLength)
    float depthFromRoomBack = roomLength - y;
    if (depthFromRoomBack < 0.0f) depthFromRoomBack = 0.0f;

    float halfWidth = roomWidth * 0.5f;

    // At the back wall (y = roomLength): depthFromRoomBack = 0.0
    // leftWallX = -halfWidth (-120.0)
    // rightWallX = +halfWidth (+120.0)
    // Total width at the back wall = 240.0 EXACT UNITS
    return -halfWidth - (SIDE_WALL_INCLINATION_SLOPE * depthFromRoomBack);
}

float GetRightWallX(float y, float roomWidth, float roomLength) {
    return -GetLeftWallX(y, roomWidth, roomLength);
}

Vector2 ProjectRoomPoint(float x, float y, float roomWidth) {
    (void)roomWidth;
    Vector2 screenPos;
    screenPos.x = x * PROJECTION_SCALE;
    screenPos.y = -(y * PROJECTION_Y_FACTOR) * PROJECTION_SCALE;
    return screenPos;
}
