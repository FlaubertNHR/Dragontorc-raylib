#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"

void UpdatePlayerPhysics(Game *game, float *outMoveX, float *outMoveY, float dt);
void UpdateServantPhysics(Game *game, float dt);
void UpdateCollisions(Game *game, float prevX, float prevY, float moveX, float moveY);
void ApplyBounce(Player *player, float forceX, float forceY);
void TransitionToRoom(Game *game, const Door *crossedDoor);
void UpdateDoorAnimations(Room *room, float dt);
void UpdateServantDoorInteraction(Game *game);
void UpdateItemPickups(Game *game);
void UpdateServantItemInteraction(Game *game, float dt);
void StartServantItemDrop(Game *game); 
void UpdateFallingItems(Game *game, float dt);

#endif // PHYSICS_H
