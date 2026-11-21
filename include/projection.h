#ifndef PROJECTION_H
#define PROJECTION_H

#include "raylib.h"

// Returns the logical X boundary of the left wall at depth y for a specific room length
float GetLeftWallX(float y, float roomWidth, float roomLength);

// Returns the logical X boundary of the right wall at depth y for a specific room length
float GetRightWallX(float y, float roomWidth, float roomLength);

// Projects logical room coordinates (x, y) into 2.5D trapezoidal screen coordinates
Vector2 ProjectRoomPoint(float x, float y, float roomWidth);

#endif // PROJECTION_H
