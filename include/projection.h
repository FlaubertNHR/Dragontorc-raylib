#ifndef PROJECTION_H
#define PROJECTION_H

#include "raylib.h"

// Projects logical room coordinates (x, y) into 2.5D trapezoidal screen coordinates
Vector2 ProjectRoomPoint(float x, float y, float roomWidth);

#endif // PROJECTION_H
