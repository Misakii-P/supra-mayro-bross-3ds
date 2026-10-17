// Super Mayo Boss - Legend of the Lost Kingdom
// Entry point: title / level / death (debug).

#include "title.h"
#include "level.h"
#include "level2.h"
#include "level3.h"
#include "death.h"
#include "game.h"
#include "font.h"
#include "audio.h"
#include "screen.h"

#include <citro2d.h>

int g_deathCount = 0;

static int scene = SCENE_TITLE;
static int lastPlayedScene = SCENE_LEVEL;   // where to return after death

//---------------------------------------------------------------------------------
// Konami code: Up, Up, Down, Down, Left, Right, Left, Right, B, A.
// D-pad only - the circle pad must not count.
//---------------------------------------------------------------------------------
static const u32 kCheatSeq[10] = {
	KEY_DUP,   KEY_DUP,
	KEY_DDOWN, KEY_DDOWN,
	KEY_DLEFT, KEY_DRIGHT, KEY_DLEFT, KEY_DRIGHT,
	KEY_B,     KEY_A
};
#define CHEAT_LEN 10
// Only these bits participate. KEY_UP/DOWN/LEFT/RIGHT are deliberately absent:
// the circle pad is KEY_CPAD_*, which must not count. Circle-pad input is
// masked out entirely, so pushing the stick neither advances nor aborts.
#define CHEAT_MASK (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT | KEY_A | KEY_B)
static int cheatPos = 0;
static int cheatIdle = 0;
#define CHEAT_TIMEOUT 60   // frames between presses before the sequence resets

static int cheat_push(u32 kDown)
{
	kDown &= CHEAT_MASK;
	if (kDown == 0)
		return 0;   // idle frame, or non-participating input: leave state alone

	// Time out and restart if the next press takes too long.
	if (cheatPos > 0 && ++cheatIdle > CHEAT_TIMEOUT)
	{
		cheatPos = 0;
		cheatIdle = 0;
	}

	// Compare against the key at the current position only. Searching forward
	// for any matching key in the sequence breaks on repeated inputs, because
	// the earlier duplicate always matches first.
	if (kDown & kCheatSeq[cheatPos])
	{
		cheatPos++;
		cheatIdle = 0;
		if (cheatPos == CHEAT_LEN)
		{
			cheatPos = 0;
			return 1;
		}
		return 0;
	}

	// Any other participating button aborts the sequence.
	cheatPos = 0;
	cheatIdle = 0;
	return 0;
}

//---------------------------------------------------------------------------------
// Tear down the current scene and switch to another one.
//---------------------------------------------------------------------------------
static void scene_goto(int next)
{
	if (next == scene)
		return;

	if (scene == SCENE_TITLE)        title_exit();
	else if (scene == SCENE_LEVEL)   level_exit();
	else if (scene == SCENE_LEVEL2)  level2_exit();
	else if (scene == SCENE_BOSS)    level3_exit();
	else                             death_exit();

	scene = next;

	if (scene == SCENE_TITLE)        title_init();
	else if (scene == SCENE_LEVEL)   level_init();
	else if (scene == SCENE_LEVEL2)  level2_init();
	else if (scene == SCENE_BOSS)    level3_init();
	else                             death_init();
}

static void gfx_setup(void)
{
	gfxInitDefault();
	ndspInit();

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	gfx_setup();
	romfsInit();
	if (!screen_init())
		svcBreak(USERBREAK_PANIC);
	font_init();

	if (scene == SCENE_TITLE)
		title_init();
	else if (scene == SCENE_LEVEL)
		level_init();
	else if (scene == SCENE_LEVEL2)
		level2_init();
	else if (scene == SCENE_BOSS)
		level3_init();
	else
		death_init();

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		touchPosition touch;
		hidTouchRead(&touch);

		if (cheat_push(kDown))
		{
			if (scene == SCENE_TITLE)       scene_goto(SCENE_LEVEL);
			else if (scene == SCENE_LEVEL)  scene_goto(SCENE_LEVEL2);
			else if (scene == SCENE_LEVEL2) scene_goto(SCENE_BOSS);
			else if (scene == SCENE_BOSS)   scene_goto(SCENE_LEVEL);
			// The scene just changed: do not let the same input press fall
			// through into the new scene's update handler.
			continue;
		}

		if (scene == SCENE_TITLE)
		{
			int req = title_update(kDown, touch);
			if (req == 1)
				scene_goto(SCENE_LEVEL);
		}
		else if (scene == SCENE_LEVEL2)
		{
			int r = level2_update(kDown);
			if (r == 1)
				level2_restart();
			else if (r == 2)
			{
				lastPlayedScene = SCENE_LEVEL2;
				scene_goto(SCENE_DEATH);
			}
			else if (r == 3)
				scene_goto(SCENE_BOSS);     // reached the goal
		}
		else if (scene == SCENE_BOSS)
		{
			int r = level3_update(kDown);
			if (r == 1)
				level3_restart();
			else if (r == 2)
			{
				lastPlayedScene = SCENE_BOSS;
				scene_goto(SCENE_DEATH);
			}
		}
		else if (scene == SCENE_LEVEL)
		{
			int r = level_update(kDown);
			if (r == 1)
			{
				// Restart level (beginning) without reallocating.
				level_restart();
			}
			else if (r == 2)
			{
				lastPlayedScene = SCENE_LEVEL;
				scene_goto(SCENE_DEATH);
			}
			else if (r == 3)
				scene_goto(SCENE_LEVEL2);   // reached the goal
		}
		else
		{
			// Death screen: START restarts the level (after delay).
			if (death_ready() && (kDown & KEY_START))
				scene_goto(lastPlayedScene);
		}

		audio_update();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		if (scene == SCENE_TITLE)
			title_draw();
		else if (scene == SCENE_LEVEL)
			level_draw();
		else if (scene == SCENE_LEVEL2)
			level2_draw();
		else if (scene == SCENE_BOSS)
			level3_draw();
		else
			death_draw();
		C3D_FrameEnd(0);
	}

	if (scene == SCENE_TITLE)
		title_exit();
	else if (scene == SCENE_LEVEL)
		level_exit();
	else if (scene == SCENE_LEVEL2)
		level2_exit();
	else if (scene == SCENE_BOSS)
		level3_exit();
	else
		death_exit();

	font_exit();
	romfsExit();
	C2D_Fini();
	C3D_Fini();
	ndspExit();
	gfxExit();
	return 0;
}
