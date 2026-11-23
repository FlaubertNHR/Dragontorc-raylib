#include "map_data.h"
#include <stddef.h>
#include "config.h"

int LoadMapData(Room rooms[], int maxRooms) {
    if (maxRooms < 3) return 0;

    // =========================================================================
    // ROOM 0: Citadel Entrance
    // =========================================================================
    rooms[0] = (Room){
        .width = 360.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = RED,
        .name = "CAVERNS OF DOOM",
        .doors = {
            { 10, WALL_BACK,     POS_CENTER, 60.0f, 85.0f, RED,    2, 30, DOOR_CLOSED, 0.0f },
            { 11, WALL_RIGHT,  60.0f, 35.0f, 85.0f, MAROON, 1, 20, DOOR_OPEN, 1.0f },
            { 12, WALL_RIGHT, 160.0f, 35.0f, 85.0f, MAROON, 1, 21, DOOR_FREE_EXIT, 1.0f } 
        },
        .doorCount = 3,
    .items = {
        // Object hung right in the center of the back wall
        WALL_ITEM_BACK(103, ITEM_TYPE_CROWN, "Crown of Dumnonia", POS_CENTER, 65.0f, YELLOW),

        // Pair of objects on the ground, positioned symmetrically to the left and right (-80 and +80)
        FLOOR_ITEM(101, ITEM_TYPE_KEY,    "Iron Key",     POS_LEFT(80.0f),  80.0f, GOLD),
        FLOOR_ITEM(102, ITEM_TYPE_SCROLL, "Spell Scroll", POS_RIGHT(80.0f), 80.0f, SKYBLUE),

        // Torches / Candelabra flanking the back door (approx. 50 units from the center of the door)
        WALL_ITEM_BACK(107, ITEM_TYPE_ARTIFACT, "Torch Left",  FLANK_LEFT(POS_CENTER, 50.0f),  50.0f, ORANGE),
        WALL_ITEM_BACK(108, ITEM_TYPE_ARTIFACT, "Torch Right", FLANK_RIGHT(POS_CENTER, 50.0f), 50.0f, ORANGE)
    },
        .itemCount = 5        
    };

    
    // =========================================================================
    // ROOM 1: West Corridor
    // =========================================================================
    rooms[1] = (Room){
        .width = 240.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = GREEN,
        .name = "GATEHOUSE LEVEL",
        .doors = {
            { 20, WALL_LEFT,   60.0f, 35.0f, 85.0f, MAROON, 0, 11, DOOR_OPEN, 1.0f },
            { 21, WALL_LEFT,  160.0f, 35.0f, 85.0f, MAROON, 0, 12, DOOR_OPEN, 1.0f  },
            { 22, WALL_RIGHT, 110.0f, 35.0f, 85.0f, RED,    2, 31, DOOR_OPEN, 1.0f }
        },
        .doorCount = 3,
        .items = {
            WALL_ITEM_BACK(104, ITEM_TYPE_TREASURE,    "Gold Chalice",     POS_RIGHT(80),  60.0f, ORANGE),
            FLOOR_ITEM(105, ITEM_TYPE_KEY,    "Silver Key",     POS_RIGHT(60),  140.0f, LIGHTGRAY)
        },
        .itemCount = 2        
    };

    // =========================================================================
    // ROOM 2: The Crossroads
    // =========================================================================
    rooms[2] = (Room){
        .width = 560.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = BLUE,
        .name = "THE CROSSROADS",
        .doors = {
            { 30, WALL_BACK,    0.0f, 60.0f, 85.0f, RED, 0, 10, DOOR_OPEN, 1.0f },
            { 31, WALL_BACK, -200.0f, 60.0f, 85.0f, RED, 1, 22, DOOR_OPEN, 1.0f }
        },
        .doorCount = 2,
        .items = {
            FLOOR_ITEM(106, ITEM_TYPE_ARTIFACT,     "Orb of Power",     POS_CENTER,  110.0f, PURPLE)
        },
        .itemCount = 1        
    };

    return 3;
}
