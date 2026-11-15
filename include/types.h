#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include "config.h"

typedef enum {
    WALL_LEFT,
    WALL_BACK,
    WALL_RIGHT
} WallType;

typedef enum {
    DOOR_CLOSED,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_CLOSING,
    DOOR_FREE_EXIT
} DoorState;

typedef enum {
    RESULT_SPAWN_ITEM,     // Spawns a new item
    RESULT_ACTIVATE_FIXTURE,// Changes status of a stationary object (e.g., lights a bonfire)
    RESULT_UNLOCK_DOOR,     // Unlocks a door
    RESULT_LEARN_SPELL      // Gives spell
} ReactionType;

typedef struct {
    int id;
    WallType wall;
    float position;
    float width;
    float height;
    Color color;

    int targetRoom;
    int targetDoorId;
    DoorState state;
    float openProgress; // 0.0f = closed, 1.0f = totally open
} Door;

typedef enum {
    ITEM_TYPE_KEY,       // Opens an specific door/lock
    ITEM_TYPE_SCROLL,    // Magic scroll
    ITEM_TYPE_CROWN,      // One of the 5 Royal Crowns of Britain
    ITEM_TYPE_TREASURE,   // Treasure/relic
    ITEM_TYPE_ARTIFACT    // Magic object with special effect
} ItemCategory;

typedef struct {
    int id;                   // Absolute unique ID of an object (e.g., 101, 102...)
    ItemCategory category;    // Visual&Lgic catgory
    const char *name;         // "Iron Key of Locris", "Crown of Dumnonia", etc.
    float x;                  // Position on the room floor
    float y;

    bool isWallItem;          // true = colgado en la pared, false = en el suelo
    WallType wall;            // WALL_LEFT, WALL_BACK, WALL_RIGHT (si isWallItem == true)
    float wallHeightOffset;   // Elevation height above the wall (if isWallItem == true)   

    bool active;              // true = present in the room, false = collected / inactive
    Color color;              // Identifying theme color
    int targetId;             // ID of the element being interacted with (door, pedestal, etc.)
    
} Item;

typedef struct {
    float width;
    float length;
    float wallHeight;
    Color wallColor;
    const char *name;
    Door doors[MAX_DOORS_PER_ROOM];
    int doorCount;
    // Objetos en el suelo 
    Item items[MAX_ITEMS_PER_ROOM];
    int itemCount;      
} Room;

/*
* **Fixed elements**: Rendered as solid 2.5D structures with a base projected onto the ground and a vertical height (e.g., a fireplace with brickwork and animated flames when active, a pedestal with a stone rim, etc.).
* **Ground objects**: Small icons that hover slightly, with their shadows cast on the floor.
*/

typedef struct {
    int toolItemId;         // ID of the object you are interacting with (in hand or on the ground)
    int targetId;           // ID of the target (another Item, a Fixture, or a Door)
    bool targetIsFixture;   // Is the target a fixed object or an item on the ground?
    
    bool consumeTool;       // Is the object used destroyed?
    bool consumeTarget;     // Is the target object destroyed?

    ReactionType resultType;
    int resultData;        // ID of the newly generated item, or ID of the opened door, etc.
} InteractionRule;

/*
#### Examples of rules in `src/map_data.c`:
1. **Dry branch (ID 101) + Unlit fireplace (ID 202)**:
   * Consumes the branch (`consumeTool = true`).
   * Sets the fireplace to an ignited state (`resultType = RESULT_ACTIVATE_FIXTURE`, `state = 1`, `providesHealing = true`).
2. **Blue gem (ID 102) + Ancient pedestal (ID 202)**:
   * Places the gem and causes the *Crypt Key* (ID 105) to appear (`RESULT_SPAWN_ITEM`).
3. **Empty potion bottle (ID 103) + Moon herb on the ground (ID 104)**:
   * When the herb is dropped and touched with the potion bottle, they combine to create a *Protection Elixir* (`RESULT_SPAWN_ITEM`).
4. **Master key (ID 105) + Iron door (ID 10)**:
   * Unlocks the door (`RESULT_UNLOCK_DOOR`).
*/

typedef struct {
    float x;
    float y;
    float z;
    float speed;

    float levitationTimer;
    float floatingOffset;

    float bounceForceX;
    float bounceForceY;
    float bounceLockoutTimer;

    Item inventory[MAX_INVENTORY_ITEMS];
    int inventoryCount;
    int pickupLockoutItemId;
} Player;

typedef enum {
    CONTROL_MAROC,      // Normal control for Maroc
    CONTROL_SERVANT     // Maroc, motionless and in a trance, controlling the servant
} ControlMode;

typedef struct {
    bool active;             // Is the servant summoned?
    float x;                 // Logical room X (for floor item/door interactions)
    float y;                 // Logical room Y (clamped between 0 and room->length)
    float screenX;           // Absolute screen X position
    float screenY;           // Absolute screen Y position
    float speed;             // Movement speed in screen pixels
    float pulseTime;         // Timer for glowing animation
    int carriedItemId;       // Carried item ID (0 if empty)
} Servant;

typedef struct {
    Player player;
    Servant servant;
    ControlMode controlMode;   
    Room rooms[MAX_ROOMS];
    int roomCount;
    int currentRoom;

    Vector2 projectedPlayer;
    Vector2 cameraScroll;
    
    Texture2D itemTexture;    // Spritesheet atlas for item graphics  
    Texture2D frameTexture;    // Decorative frame texture PNG (1024x768)
} Game;

#endif // TYPES_H
