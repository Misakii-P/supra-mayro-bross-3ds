// Level 2: side-scroller with sliding evil Gumbas.

#include "level2.h"
#include "player.h"
#include "draw.h"
#include "game.h"
#include "font.h"
#include "audio.h"
#include "screen.h"

#include <citro2d.h>
#include <stdio.h>

#define LEVEL2_0_IMG  "romfs:/gfx/level2_0.t3x"
#define LEVEL2_1_IMG  "romfs:/gfx/level2_1.t3x"
#define LEVEL2_2_IMG  "romfs:/gfx/level2_2.t3x"
#define BOTTOM_BG_IMG "romfs:/gfx/bottomgame.t3x"

static C2D_SpriteSheet sheets[3];
static C2D_Image       images[3];

static C2D_SpriteSheet enemySheet;
static C2D_Image       enemyImage;

static C2D_SpriteSheet bottomBgSheet;
static C2D_Image       bottomBgImage;
static C2D_SpriteSheet miniMayroSheet;
static C2D_Image       miniMayroImage;

static Player player;
static float scrollX = 0;

static int restartCooldown = 0;


static C2D_TextBuf textBuf;
static C2D_Text    levelText;
static C2D_TextBuf counterBuf;
static C2D_Text    counterText;
static int lastCount = -1;

// Evil Gumbas: slide along their red zone, floating (no gravity).
static struct { float x; float vx; float y; } gumbas[LEVEL2_NUM_GUMBAS];
#define GUMBA_SPEED 2.0f

void level2_init(void)
{
	sheets[0] = C2D_SpriteSheetLoad(LEVEL2_0_IMG);
	sheets[1] = C2D_SpriteSheetLoad(LEVEL2_1_IMG);
	sheets[2] = C2D_SpriteSheetLoad(LEVEL2_2_IMG);
	for (int i = 0; i < 3; i++)
	{
		if (!sheets[i])
		{
			// Release the panels that did load before stopping, so the panic
			// cannot leak them.
			level2_exit();
			svcBreak(USERBREAK_PANIC);
			return;
		}
		images[i] = C2D_SpriteSheetGetImage(sheets[i], 0);
	}

	enemySheet = C2D_SpriteSheetLoad("romfs:/gfx/gumba.t3x");
	if (enemySheet)
		enemyImage = C2D_SpriteSheetGetImage(enemySheet, 0);

	bottomBgSheet = C2D_SpriteSheetLoad(BOTTOM_BG_IMG);
	if (bottomBgSheet)
		bottomBgImage = C2D_SpriteSheetGetImage(bottomBgSheet, 0);

	miniMayroSheet = C2D_SpriteSheetLoad("romfs:/gfx/player_walk_1.t3x");
	if (miniMayroSheet)
		miniMayroImage = C2D_SpriteSheetGetImage(miniMayroSheet, 0);

	textBuf = C2D_TextBufNew(32);
	counterBuf = C2D_TextBufNew(16);
	if (!textBuf || !counterBuf)
	{
		// Out of memory. Release everything loaded so far so the panic below
		// cannot leak, then stop instead of dereferencing NULL every frame.
		level2_exit();
		svcBreak(USERBREAK_PANIC);
		return;
	}
	if (defaultFont)
		C2D_TextFontParse(&levelText, defaultFont, textBuf, "LEVUL 2");
	else
		C2D_TextParse(&levelText, textBuf, "LEVUL 2");
	C2D_TextOptimize(&levelText);

	player_set_level(level2_platforms, LEVEL2_NUM_PLATFORMS,
	                 level2_jumpPads, LEVEL2_NUM_JUMPPADS, LEVEL2_WIDTH);
	lastCount = -1;
	player_init(&player, 90.0f, 10.0f);
	for (int i = 0; i < LEVEL2_NUM_GUMBAS; i++)
	{
		gumbas[i].x  = level2_gumbas[i].x0;
		gumbas[i].vx = GUMBA_SPEED;
		gumbas[i].y  = level2_gumbas[i].y;
	}
	scrollX = 0;
	restartCooldown = 30;
	audio_init("romfs:/audio/level2.ogg");
}

void level2_restart(void)
{
	lastCount = -1;
	player_reset(&player, 90.0f, 10.0f);
	scrollX = 0;
	restartCooldown = 30;
	for (int i = 0; i < LEVEL2_NUM_GUMBAS; i++)
	{
		gumbas[i].x  = level2_gumbas[i].x0;
		gumbas[i].vx = GUMBA_SPEED;
		gumbas[i].y  = level2_gumbas[i].y;
	}
	audio_restart();
}

int level2_update(u32 kDown)
{
	u32 kHeld = hidKeysHeld();
	if (player_update(&player, kDown, kHeld))
		g_deathCount++;

	// Evil gumbas slide along their red zones (no gravity), touch -> death.
	for (int i = 0; i < LEVEL2_NUM_GUMBAS; i++)
	{
		gumbas[i].x += gumbas[i].vx;
		if (gumbas[i].x <= level2_gumbas[i].x0) { gumbas[i].x = level2_gumbas[i].x0; gumbas[i].vx =  GUMBA_SPEED; }
		if (gumbas[i].x >= level2_gumbas[i].x1) { gumbas[i].x = level2_gumbas[i].x1; gumbas[i].vx = -GUMBA_SPEED; }

		if (player.x + PLAYER_W > gumbas[i].x + 10 && player.x < gumbas[i].x + 50 &&
		    player.y + PLAYER_HEIGHT > gumbas[i].y + 8 && player.y < gumbas[i].y + 38)
		{
			g_deathCount++;
			return 2;
		}
	}

	// Camera dead zone.
	float playerCenter = player.x + PLAYER_W / 2.0f - scrollX;
	float deadLeft  = SCREEN_TOP_W / 2.0f - 50.0f;
	float deadRight = SCREEN_TOP_W / 2.0f + 50.0f;
	if (playerCenter < deadLeft)
		scrollX -= (deadLeft - playerCenter);
	else if (playerCenter > deadRight)
		scrollX += (playerCenter - deadRight);
	if (scrollX < 0) scrollX = 0;
	if (scrollX > LEVEL2_WIDTH - SCREEN_TOP_W) scrollX = LEVEL2_WIDTH - SCREEN_TOP_W;

	// Goal: the yellow "NEXT" sign at the end of level 2 -> BOSS level.
	if (player.x + PLAYER_W > LEVEL2_GOAL_X0 && player.x < LEVEL2_GOAL_X1 &&
	    player.y + PLAYER_HEIGHT > LEVEL2_GOAL_Y0 && player.y < LEVEL2_GOAL_Y1)
		return 3;

	if (restartCooldown > 0)
	{
		restartCooldown--;
		return 0;
	}
	if (kDown & KEY_START)
	{
		g_deathCount++;
		restartCooldown = 30;
		return 1;
	}
	return 0;
}

void level2_draw(void)
{
	const u32 black = C2D_Color32(0, 0, 0, 255);
	const u32 sky   = C2D_Color32(0x36, 0x36, 0x36, 255);

	C2D_TargetClear(screenTop, sky);
	C2D_SceneBegin(screenTop);

	int panelX[3] = { 0, 730, 1460 };
	for (int i = 0; i < 3; i++)
	{
		if (!images[i].subtex)
			continue;
		float sx = panelX[i] - scrollX;
		if (sx + images[i].subtex->width > 0 && sx < SCREEN_TOP_W)
			C2D_DrawImageAt(images[i], sx, 0, 0, NULL, 1.0f, 1.0f);
	}

	// Gumbas.
	if (enemyImage.subtex)
		for (int i = 0; i < LEVEL2_NUM_GUMBAS; i++)
			C2D_DrawImageAt(enemyImage, gumbas[i].x - scrollX, gumbas[i].y, 0, NULL, 1.0f, 1.0f);

	player_draw(&player, scrollX);

	C2D_TargetClear(screenBottom, black);
	C2D_SceneBegin(screenBottom);
	if (bottomBgImage.subtex)
		C2D_DrawImageAt(bottomBgImage, 0, 0, 0, NULL, 1.0f, 1.0f);
	// Progress bar (pill) at y~62, x~20..306; mini Mayro follows progress.
	if (miniMayroImage.subtex)
	{
		float progress = player.x / (float)(LEVEL2_WIDTH - PLAYER_W);
		if (progress < 0) progress = 0;
		if (progress > 1) progress = 1;
		float pillLeft = 20.0f, pillRight = 306.0f, pillY = 62.0f;
		float mx = pillLeft + progress * (pillRight - pillLeft);
		float miniH = 32.0f;
		float mscale = miniH / 78.0f;
		C2D_DrawImageAt(miniMayroImage, mx - (46.0f*mscale)/2.0f, pillY - 8.0f, 0, NULL, mscale, mscale);
	}
	C2D_DrawText(&levelText, C2D_WithColor, 4.0f, 4.0f, 0.0f, 2.0f, 2.0f, C2D_Color32(0, 0, 0, 255));

	if (counterBuf && g_deathCount != lastCount)
	{
		lastCount = g_deathCount;
		C2D_TextBufClear(counterBuf);
		char buf[16];
		snprintf(buf, sizeof(buf), "%d LIF", g_deathCount);
		if (defaultFont)
			C2D_TextFontParse(&counterText, defaultFont, counterBuf, buf);
		else
			C2D_TextParse(&counterText, counterBuf, buf);
		C2D_TextOptimize(&counterText);
	}
	C2D_DrawText(&counterText, C2D_WithColor, 230.0f, 4.0f, 0.0f, 1.5f, 1.5f, C2D_Color32(0, 0, 0, 255));
}

void level2_exit(void)
{
	for (int i = 0; i < 3; i++)
		if (sheets[i]) { C2D_SpriteSheetFree(sheets[i]); sheets[i] = NULL; }
	if (textBuf)     { C2D_TextBufDelete(textBuf); textBuf = NULL; }
	if (counterBuf)  { C2D_TextBufDelete(counterBuf); counterBuf = NULL; }
	if (enemySheet)    { C2D_SpriteSheetFree(enemySheet); enemySheet = NULL; }
	if (bottomBgSheet)  { C2D_SpriteSheetFree(bottomBgSheet); bottomBgSheet = NULL; }
	if (miniMayroSheet) { C2D_SpriteSheetFree(miniMayroSheet); miniMayroSheet = NULL; }
	audio_exit();
	player_exit();
}
