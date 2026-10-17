#ifndef PLAYER_H
#define PLAYER_H

#include <3ds.h>
#include "levels.h"

#define PLAYER_W   18
#define PLAYER_HEIGHT   36
#define PLAYER_SCALE (43.0f / 78.0f)

// Walk animation frames are 0..2; see frameSeq in player.c.

typedef struct
{
	float x, y;       // world position (top-left of sprite)
	float vx, vy;     // velocity
	float spawnX, spawnY;  // where to respawn after falling out of the level
	int   onGround;
	int   coyoteTimer;
	int   accelTimer;
	int   wallKickTimer;
	int   wallKickDir;
	int   facing;     // 1 = right, -1 = left
	int   state;      // 0 = idle, 1 = walk
	int   animFrame;  // current frame index (0 = stand, 1..8 = walk)
	int   animTimer;  // frames until next animation step
} Player;

// Walk animation frame order: 3 unique poses over an 8-step cycle, preserving
// the original 1.6s timing (each pose held 2 steps). Shared with the death
// screen so both animate identically.
extern const int playerFrameSeq[8];

void player_set_level(const Rect *platforms, int numPlatforms,
                      const Rect *jumpPads, int numJumpPads, int levelWidth);
void player_init(Player *p, float x, float y);
void player_reset(Player *p, float x, float y);
int  player_update(Player *p, u32 kDown, u32 kHeld); // returns 1 if respawned (fell)
void player_draw(Player *p, float scrollX);
void player_exit(void);

#endif // PLAYER_H
