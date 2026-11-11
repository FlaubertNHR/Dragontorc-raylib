#include "map_data.h"
#include <stddef.h>
#include "config.h"

int LoadMapData(Room rooms[], int maxRooms) {
    if (maxRooms < 3) return 0;

    // =========================================================================
    // SALA 0: Citadel Entrance
    // =========================================================================
    rooms[0] = (Room){
        .width = 360.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = RED,
        .name = "CAVERNS OF DOOM",
        .doors = {
            { 10, WALL_BACK,  180.0f, 60.0f, 85.0f, RED,    2, 30, DOOR_CLOSED, 0.0f },
            { 11, WALL_RIGHT,  60.0f, 35.0f, 85.0f, MAROON, 1, 20, DOOR_OPEN, 1.0f },
            { 12, WALL_RIGHT, 160.0f, 35.0f, 85.0f, MAROON, 1, 21, DOOR_FREE_EXIT, 1.0f } 
        },
        .doorCount = 3,
        .items = {
            { 101, ITEM_TYPE_KEY,    "Iron Key",      100.0f,  80.0f, false, WALL_BACK,  0.0f, true, GOLD, 10 },
            { 102, ITEM_TYPE_SCROLL, "Spell Scroll",  240.0f, 120.0f, false, WALL_BACK,  0.0f, true, SKYBLUE, 0 },
            { 103, ITEM_TYPE_CROWN,  "Crown of Dumnonia", 180.0f, 150.0f, true, WALL_BACK, 65.0f, true, YELLOW, 0 } // Objeto en pared
        },
        .itemCount = 3        
    };

    // =========================================================================
    // SALA 1: West Corridor
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
            { 104, ITEM_TYPE_TREASURE, "Gold Chalice", 120.0f, 100.0f, false, WALL_BACK, 0.0f, true, ORANGE, 0 },
            { 105, ITEM_TYPE_KEY,      "Silver Key",   180.0f, 140.0f, false, WALL_BACK, 0.0f, true, LIGHTGRAY, 22 }
        },
        .itemCount = 2        
    };

    // =========================================================================
    // SALA 2: The Crossroads
    // =========================================================================
    rooms[2] = (Room){
        .width = 560.0f,
        .length = 220.0f,
        .wallHeight = 140.0f,
        .wallColor = BLUE,
        .name = "THE CROSSROADS",
        .doors = {
            { 30, WALL_BACK, 280.0f, 60.0f, 85.0f, RED, 0, 10, DOOR_OPEN, 1.0f },
            { 31, WALL_BACK,  80.0f, 60.0f, 85.0f, RED, 1, 22, DOOR_OPEN, 1.0f }
        },
        .doorCount = 2,
        .items = {
            { 106, ITEM_TYPE_ARTIFACT, "Orb of Power", 280.0f, 110.0f, false, WALL_BACK, 0.0f, true, PURPLE, 0 }
        },
        .itemCount = 1        
    };

    return 3;
}
