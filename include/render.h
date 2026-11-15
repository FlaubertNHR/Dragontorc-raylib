#ifndef RENDER_H
#define RENDER_H

#include "types.h"

void DrawFloorGrid(const Room *room, Vector2 scroll);
void DrawWalls(const Room *room, Vector2 scroll);
void DrawDoorsBehind(const Room *room, const Player *player, Vector2 scroll);
void DrawDoorsInFront(const Room *room, const Player *player, Vector2 scroll);
void DrawPlayer(const Game *game);
void DrawServant(const Servant *s, float roomWidth, Vector2 scroll, const Game *game);
void DrawHUD(const Game *game);
void DrawRoomItems(const Room *room, Vector2 scroll);
void DrawDecorativeFrame(const Game *game);

#endif // RENDER_H
