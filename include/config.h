#ifndef CONFIG_H
#define CONFIG_H

// Screen and display settings
#define SCREEN_WIDTH               800
#define SCREEN_HEIGHT              600
#define TARGET_FPS                 60
#define WINDOW_TITLE               "Dragontorc Remake"

#define FRAME_THICKNESS            (SCREEN_WIDTH / 16.0f) // Decorative outer frame (1/16 of the total width)
#define HUD_HEIGHT                 (SCREEN_HEIGHT * 0.30f) // HUD height (30% of the total height)

// Dimensions of the window/usable play area (Viewport)
#define VIEWPORT_X                 FRAME_THICKNESS
#define VIEWPORT_Y                 FRAME_THICKNESS
#define VIEWPORT_WIDTH             (SCREEN_WIDTH - (FRAME_THICKNESS * 2.0f))
#define VIEWPORT_HEIGHT            (SCREEN_HEIGHT - FRAME_THICKNESS - HUD_HEIGHT)

// Room rendering and perspective settings
#define TILE_SIZE                  64.0f
#define FIXED_VISUAL_DEPTH         350.0f

// Inclination slope of the side walls (2.5D trapezoidal perspective)
// Preserves the exact original wall angle (375 / 350)
#define SIDE_WALL_INCLINATION_SLOPE   1.07142857f
#define PROJECTION_Y_FACTOR        0.54f  // Perspective compression factor; i.e. from how high you look at the floor 
                                          // 0: no floor seen; 1: cenital view
#define PROJECTION_SCALE           1.3f
#define PROJECTION_MIN_FACTOR_Y    0.0f

// Camera tuning: Anchors Maroc's fixed screen Y position (~81% down the viewport)
#define CAMERA_VIEWPORT_CENTER_Y_RATIO 0.81f

// Player initial state
#define INITIAL_PLAYER_X           0.0f
#define INITIAL_PLAYER_Y           40.0f

// Player movement and physics
#define PLAYER_BASE_SPEED          280.0f
#define SPEED_MULTIPLIER_X         1.0f
#define DIAGONAL_FACTOR_X          0.55f

// Bounce mechanics and collision damping
#define BOUNCE_IMPULSE_FORCE       95.0f
#define BOUNCE_LOCKOUT_TIME        0.35f
#define BOUNCE_FRICTION            0.85f

// Floating / levitation aesthetics
#define PLAYER_BASE_Z              24.0f
#define PLAYER_WIDTH               20.0f
#define PLAYER_HEIGHT              35.0f
#define SHADOW_RADIUS_X            16.0f // 36
#define SHADOW_RADIUS_Y            8.0f   // 6

#define SHADOW_LOGIC_RADIUS_Y      (SHADOW_RADIUS_Y / (PROJECTION_Y_FACTOR * PROJECTION_SCALE))
#define SHADOW_LOGIC_RADIUS_X      (SHADOW_RADIUS_X / PROJECTION_SCALE)

#define LEVITATION_SPEED           4.0f // Bobbing frequency
#define LEVITATION_AMPLITUDE       3.5f

// Servant parameters
#define SERVANT_BASE_SPEED         260.0f
#define SERVANT_HEAD_OFFSET        30.0f

// Door transitions & collision thresholds
#define SIDE_WALL_DETECTION_MARGIN     0.0f   // 8
#define DOOR_TRANSITION_STEP_X         8.0f  // 2?
#define WALL_BOUNCE_IMPULSE_FORCE      50.0f
#define DOOR_TRIGGER_THRESHOLD_SIDE    3.0f
#define DOOR_TRIGGER_THRESHOLD_BACK    8.0f
#define DOOR_TRANSITION_BACK_MARGIN_Y  30.0f

// World capacity limits
#define MAX_DOORS_PER_ROOM         6
#define MAX_ROOMS                  3

// Items characteristics
#define MAX_ITEMS_PER_ROOM         8
#define MAX_INVENTORY_ITEMS        3

#endif // CONFIG_H
