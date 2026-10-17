// Player: physics, input, animation.

#include "player.h"
#include "draw.h"

#include <citro2d.h>
#include <stdio.h>

#define PLAYER_WALK_PREFIX "romfs:/gfx/player_walk_"

// Physics constants (pixels per frame).
#define GRAVITY     0.45f
#define MOVE_SPEED  6.0f
#define ACCEL       0.05f
#define ACCEL_GROW  0.015f
#define ACCEL_MAX   0.6f
#define FRICTION    0.88f
// Speed below which the player snaps to a dead stop. Kept low so the creep
// decays gradually instead of being cut off mid-slide.
#define STOP_SPEED  0.05f
// Speed imparted when slamming into a wall.
#define WALL_BOUNCE   4.5f
// Speed imparted by a wall jump (A pressed within WALL_KICK_FRAMES of a bump).
#define WALL_JUMP_HORZ 5.0f
#define JUMP_VEL   -7.0f
#define MAX_FALL    8.0f
#define COYOTE_FRAMES  6   // ~0.1s grace to jump after leaving a ledge
#define WALL_KICK_FRAMES 10  // grace to wall-jump after bumping a wall (~0.17s)
#define ANIM_FRAMES  3  // frames per animation step (~0.05s at 60fps)

// Jump pad launch velocity.
#define JUMP_PAD_VEL  -8.0f

// Active level geometry (set by player_set_level).
static const Rect *s_platforms;
static int s_numPlatforms;
static const Rect *s_jumpPads;
static int s_numJumpPads;
static int s_levelWidth;

void player_set_level(const Rect *platforms, int numPlatforms,
                      const Rect *jumpPads, int numJumpPads, int levelWidth)
{
	s_platforms = platforms;
	s_numPlatforms = numPlatforms;
	s_jumpPads = jumpPads;
	s_numJumpPads = numJumpPads;
	s_levelWidth = levelWidth;
}

// 3 unique walk frames (deduped from 8). Frame sequence preserves the original
// 1.6s cycle timing (each unique pose held 0.4s = 2 steps).
static C2D_SpriteSheet walkSheets[3];
static C2D_Image       walkImages[3];

const int playerFrameSeq[8] = { 0, 0, 1, 1, 2, 2, 1, 1 };

void player_reset(Player *p, float x, float y)
{
	p->x = x;
	p->y = y;
	p->spawnX = x;
	p->spawnY = y;
	p->vx = 0;
	p->vy = 0;
	p->onGround = 0;
	p->coyoteTimer = 0;
	p->accelTimer = 0;
	p->wallKickTimer = 0;
	p->wallKickDir = 0;
	p->facing = 1;
	p->state = 0;
	p->animFrame = 0;
	p->animTimer = ANIM_FRAMES;
}

void player_init(Player *p, float x, float y)
{
	player_reset(p, x, y);
	for (int i = 0; i < 3; i++)
	{
		char path[64];
		snprintf(path, sizeof(path), "%s%d.t3x", PLAYER_WALK_PREFIX, i + 1);
		walkSheets[i] = C2D_SpriteSheetLoad(path);
		if (walkSheets[i])
			walkImages[i] = C2D_SpriteSheetGetImage(walkSheets[i], 0);
	}
}

int player_update(Player *p, u32 kDown, u32 kHeld)
{
	(void)kDown;

	// Horizontal input with progressive acceleration (ice-like).
	if (kHeld & (KEY_DLEFT | KEY_LEFT))
	{
		if (p->facing != -1) p->accelTimer = 0; // direction change resets ramp
		p->facing = -1;
		p->accelTimer++;
		float accel = ACCEL + p->accelTimer * ACCEL_GROW;
		if (accel > ACCEL_MAX) accel = ACCEL_MAX;
		p->vx -= accel;
		if (p->vx < -MOVE_SPEED) p->vx = -MOVE_SPEED;
		p->state = 1;
	}
	else if (kHeld & (KEY_DRIGHT | KEY_RIGHT))
	{
		if (p->facing != 1) p->accelTimer = 0;
		p->facing = 1;
		p->accelTimer++;
		float accel = ACCEL + p->accelTimer * ACCEL_GROW;
		if (accel > ACCEL_MAX) accel = ACCEL_MAX;
		p->vx += accel;
		if (p->vx > MOVE_SPEED) p->vx = MOVE_SPEED;
		p->state = 1;
	}
	else
	{
		// Friction (sliding) when no input.
		p->vx *= FRICTION;
		if (p->vx > -STOP_SPEED && p->vx < STOP_SPEED) p->vx = 0;
		p->state = (p->vx != 0) ? 1 : 0;
		p->accelTimer = 0;
	}

	// Jump.
	if (kDown & KEY_A)
	{
		if (p->coyoteTimer > 0)
		{
			p->vy = JUMP_VEL;
			p->onGround = 0;
			p->coyoteTimer = 0;
			p->state = 1;
		}
		else if (p->wallKickTimer > 0)
		{
			p->vx = p->wallKickDir * WALL_JUMP_HORZ;
			p->vy = -7.0f;
			p->onGround = 0;
			p->wallKickTimer = 0;
			p->coyoteTimer = 0;
			p->state = 1;
		}
	}
	else if ((kHeld & KEY_A) && p->onGround)
	{
		// Keep jumping as long as A is held.
		p->vy = JUMP_VEL;
		p->onGround = 0;
		p->coyoteTimer = 0;
		p->state = 1;
	}
	else if ((kHeld & KEY_A) && p->wallKickTimer > 0 && !p->onGround)
	{
		// Wall kick while holding A.
		p->vx = p->wallKickDir * WALL_JUMP_HORZ;
		p->vy = -7.0f;
		p->onGround = 0;
		p->wallKickTimer = 0;
		p->coyoteTimer = 0;
		p->state = 1;
	}

	// Gravity.
	p->vy += GRAVITY;
	if (p->vy > MAX_FALL) p->vy = MAX_FALL;

	// --- Vertical collision ---
	p->onGround = 0;
	float newY = p->y + p->vy;
	// Jump pads: launch on contact.
	for (int i = 0; i < s_numJumpPads; i++)
	{
		const Rect *r = &s_jumpPads[i];
		// Only launch when Mayro's center is over the pad (top contact).
		float cx = p->x + PLAYER_W / 2.0f;
		if (cx >= r->x0 && cx <= r->x1)
		{
			if (p->vy > 0 && p->y + PLAYER_HEIGHT <= r->y0 && newY + PLAYER_HEIGHT > r->y0)
			{
				newY = r->y0 - PLAYER_HEIGHT;
				p->vy = JUMP_PAD_VEL;
				p->onGround = 0;
				p->coyoteTimer = 0;
			}
		}
	}
	for (int i = 0; i < s_numPlatforms; i++)
	{
		const Rect *r = &s_platforms[i];
		if (p->x + PLAYER_W > r->x0 && p->x < r->x1)
		{
			if (p->vy > 0 && p->y + PLAYER_HEIGHT <= r->y0 && newY + PLAYER_HEIGHT > r->y0)
			{
				newY = r->y0 - PLAYER_HEIGHT;
				p->vy = 0;
				p->onGround = 1;
			}
			else if (p->vy < 0 && p->y >= r->y1 && newY < r->y1)
			{
				newY = r->y1;
				p->vy = 0;
			}
		}
	}
	p->y = newY;

	// Coyote time: refresh while grounded, decay in air.
	if (p->onGround)
	{
		p->coyoteTimer = COYOTE_FRAMES;
		p->wallKickTimer = 0;
	}
	else
	{
		if (p->coyoteTimer > 0) p->coyoteTimer--;
		if (p->wallKickTimer > 0) p->wallKickTimer--;
	}

	// --- Horizontal collision ---
	float newX = p->x + p->vx;
	// Jump pads do NOT block horizontally (pass through sides).
	for (int i = 0; i < s_numPlatforms; i++)
	{
		const Rect *r = &s_platforms[i];
		if (p->y + PLAYER_HEIGHT > r->y0 && p->y < r->y1)
		{
			if (p->vx > 0 && p->x + PLAYER_W <= r->x0 && newX + PLAYER_W > r->x0)
			{
				newX = r->x0 - PLAYER_W;
				p->vx = -WALL_BOUNCE;
				p->wallKickTimer = WALL_KICK_FRAMES;
				p->wallKickDir = -1; // wall on right, kick left
			}
			else if (p->vx < 0 && p->x >= r->x1 && newX < r->x1)
			{
				newX = r->x1;
				p->vx = WALL_BOUNCE;
				p->wallKickTimer = WALL_KICK_FRAMES;
				p->wallKickDir = 1; // wall on left, kick right
			}
		}
	}
	p->x = newX;

	// Left border or bottom: respawn to this level's spawn point (not level 1's).
	if (p->x < 0 || p->y + PLAYER_HEIGHT > 240)
	{
		p->x = p->spawnX;
		p->y = p->spawnY;
		p->vx = 0;
		p->vy = 0;
		p->onGround = 0;
		p->coyoteTimer = 0;
		p->accelTimer = 0;
		p->wallKickTimer = 0;
		p->wallKickDir = 0;
		p->animFrame = 0;
		p->animTimer = ANIM_FRAMES;
		return 1;
	}
	// Right border: clamp (can't walk past level end).
	// Guarded because s_levelWidth is 0 until player_set_level() runs; without
	// this the clamp would push x negative and respawn instantly.
	if (s_levelWidth > PLAYER_W && p->x > s_levelWidth - PLAYER_W)
		p->x = s_levelWidth - PLAYER_W;
	// Top: no clamp, Mayro can jump past the top of the screen.

	// Animation timer.
	// Animation always advances, even when idle.
	p->animTimer--;
	if (p->animTimer <= 0)
	{
		p->animTimer = ANIM_FRAMES;
		p->animFrame++;
		if (p->animFrame > 7) p->animFrame = 0;
	}
	return 0;
}

void player_draw(Player *p, float scrollX)
{
	int idx = playerFrameSeq[p->animFrame];
	C2D_Image img = walkImages[idx];
	if (!img.subtex)
		return;

	// No horizontal flip: always face right.
	C2D_DrawImageAt(img, p->x - scrollX, p->y, 0, NULL, PLAYER_SCALE, PLAYER_SCALE);
}

void player_exit(void)
{
	for (int i = 0; i < 3; i++)
	{
		if (walkSheets[i])
		{
			C2D_SpriteSheetFree(walkSheets[i]);
			walkSheets[i] = NULL;
		}
	}
}
