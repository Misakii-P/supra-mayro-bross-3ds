// Death screen: shown whenever the player dies in a level.

#include "death.h"
#include "player.h"
#include "draw.h"
#include "audio.h"
#include "screen.h"

#include <citro2d.h>

#define DEATH_IMG "romfs:/gfx/death.t3x"

static C2D_SpriteSheet deathSheet;
static C2D_Image       deathImage;

static C2D_SpriteSheet walkSheets[3];
static C2D_Image       walkImages[3];

static int animFrame = 0;
static int animTimer = 4;
static int exitDelay = 30;

void death_init(void)
{
	deathSheet = C2D_SpriteSheetLoad(DEATH_IMG);
	if (deathSheet)
		deathImage = C2D_SpriteSheetGetImage(deathSheet, 0);

	for (int i = 0; i < 3; i++)
	{
		char path[64];
		snprintf(path, sizeof(path), "romfs:/gfx/player_walk_%d.t3x", i + 1);
		walkSheets[i] = C2D_SpriteSheetLoad(path);
		if (walkSheets[i])
			walkImages[i] = C2D_SpriteSheetGetImage(walkSheets[i], 0);
	}

	animFrame = 0;
	animTimer = 4;
	exitDelay = 30;
	audio_init("romfs:/audio/death.ogg");
}

void death_draw(void)
{
	const u32 black = C2D_Color32(0, 0, 0, 255);
	const u32 red   = C2D_Color32(255, 0, 0, 255);

	// Top: death screen image.
	C2D_TargetClear(screenTop, black);
	C2D_SceneBegin(screenTop);
	if (deathImage.subtex)
		C2D_DrawImageAt(deathImage, 0, 0, 0, NULL, 1.0f, 1.0f);

	// Bottom: animated Mayro, centered, bigger.
	C2D_TargetClear(screenBottom, red);
	C2D_SceneBegin(screenBottom);

	// Exit delay: don't accept input immediately.
	if (exitDelay > 0) exitDelay--;

	// Advance animation.
	animTimer--;
	if (animTimer <= 0)
	{
		animTimer = 4;
		animFrame++;
		if (animFrame > 7) animFrame = 0;
	}
	int idx = playerFrameSeq[animFrame];
	C2D_Image img = walkImages[idx];
	if (img.subtex)
	{
		float miniH = 96.0f;
		float mscale = miniH / 78.0f;
		float mx = 320.0f / 2.0f;
		float my = 240.0f / 2.0f;
		C2D_DrawImageAt(img, mx - (46.0f * mscale) / 2.0f, my - miniH / 2.0f, 0, NULL, mscale, mscale);
	}
}

int death_ready(void)
{
	return exitDelay == 0;
}

void death_exit(void)
{
	audio_exit();
	if (deathSheet)
	{
		C2D_SpriteSheetFree(deathSheet);
		deathSheet = NULL;
	}
	for (int i = 0; i < 3; i++)
	{
		if (walkSheets[i])
		{
			C2D_SpriteSheetFree(walkSheets[i]);
			walkSheets[i] = NULL;
		}
	}
}
