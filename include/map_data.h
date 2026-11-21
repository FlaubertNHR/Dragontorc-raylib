#ifndef MAP_DATA_H
#define MAP_DATA_H

#include "types.h"

// =========================================================================
// SYMMETRIC POSITIONING HELPERS (X-axis centered at 0.0f)
// =========================================================================

// Direct positioning from room center (X = 0.0f)
#define POS_CENTER                     0.0f
#define POS_LEFT(dist)                (-(dist))
#define POS_RIGHT(dist)               (+(dist))

// Symmetric flanking around a reference point (e.g., torches flanking a door)
#define FLANK_LEFT(point, offset)     ((point) - (offset))
#define FLANK_RIGHT(point, offset)    ((point) + (offset))

// =========================================================================
// ITEM & DECORATION QUICK CONSTRUCTORS
// =========================================================================

// Floor item constructor: (id, category, name, x, y, color)
#define FLOOR_ITEM(id, cat, name, x, y, color) \
    { (id), (cat), (name), (x), (y), false, WALL_BACK, 0.0f, true, (color), 0 }

// Wall-mounted item constructor on the back wall: (id, category, name, x, wallHeightOffset, color)
// Height is measured vertically above the floor line of the back wall (y = roomLength)
#define WALL_ITEM_BACK(id, cat, name, x, wallHeight, color) \
    { (id), (cat), (name), (x), 0.0f, true, WALL_BACK, (wallHeight), true, (color), 0 }

// Initializes room array and returns the configured room count
int LoadMapData(Room rooms[], int maxRooms);

#endif // MAP_DATA_H
