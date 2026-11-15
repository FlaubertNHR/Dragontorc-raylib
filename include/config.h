#ifndef CONFIG_H
#define CONFIG_H

// Screen and display settings
#define SCREEN_WIDTH               800
#define SCREEN_HEIGHT              600
#define TARGET_FPS                 60
#define WINDOW_TITLE               "Dragontorc Remake"

// Room rendering and perspective settings
#define TILE_SIZE                  64.0f
#define FIXED_VISUAL_DEPTH         350.0f

// Player movement and physics
#define PLAYER_BASE_SPEED          280.0f
#define SPEED_MULTIPLIER_X         1.35f
#define DIAGONAL_FACTOR_X          0.55f

// Bounce mechanics and collision damping
#define BOUNCE_IMPULSE_FORCE       95.0f
#define BOUNCE_LOCKOUT_TIME        0.35f
#define BOUNCE_FRICTION            0.85f

// Floating / levitation aesthetics
#define PLAYER_BASE_Z              24.0f
#define PLAYER_WIDTH               20.0f
#define PLAYER_HEIGHT              35.0f
#define SHADOW_RADIUS_X            16.0f
#define SHADOW_RADIUS_Y            8.0f

#define SHADOW_LOGIC_RADIUS_Y      (SHADOW_RADIUS_Y / 1.55f)
#define SHADOW_LOGIC_RADIUS_X      (SHADOW_RADIUS_X * 0.75f)

// World capacity limits
#define MAX_DOORS_PER_ROOM         6
#define MAX_ROOMS                  3

// Items characteristics
#define MAX_ITEMS_PER_ROOM   8
#define MAX_INVENTORY_ITEMS  3

#endif // CONFIG_H
