#ifndef GAME_H
#define GAME_H

#include <3ds.h>

#define SCREEN_TOP_W     400
#define SCREEN_TOP_H     240
#define SCREEN_BOTTOM_W  320
#define SCREEN_BOTTOM_H  240

// Total deaths/restarts across all levels (shared by every level scene).
extern int g_deathCount;

typedef enum
{
	SCENE_TITLE,
	SCENE_LEVEL,
	SCENE_DEATH,
	SCENE_LEVEL2,
	SCENE_BOSS,
	SCENE_COUNT
} Scene;

#endif // GAME_H
