// Level 1: side-scrolling level with player physics.

#include "level.h"
#include "player.h"
#include "draw.h"
#include "game.h"
#include "font.h"
#include "audio.h"
#include "screen.h"

#include <citro2d.h>
#include <stdio.h>

#define LEVEL1_0_IMG  "romfs:/gfx/level1_0.t3x"
#define LEVEL1_1_IMG  "romfs:/gfx/level1_1.t3x"
#define LEVEL1_2_IMG  "romfs:/gfx/level1_2.t3x"
#define BOTTOM_BG_IMG "romfs:/gfx/bottomgame.t3x"
#define CLOUDS_IMG     "romfs:/gfx/clouds.t3x"


static C2D_SpriteSheet sheets[3];
static C2D_Image       images[3];

static C2D_SpriteSheet bottomBgSheet;
static C2D_Image       bottomBgImage;

static C2D_SpriteSheet cloudsSheet;
static C2D_Image       cloudsImage;
static float           cloudOffset = 0;
static C2D_SpriteSheet miniMayroSheet;
static C2D_Image       miniMayroImage;

// Enemy (Gumba)
static C2D_SpriteSheet enemySheet;
static C2D_Image       enemyImage;
static float enemyX = 1658.0f, enemyY = 56.0f;
static float enemyVX = 3.0f;
static float enemyLeft = 1658.0f, enemyRight = 1775.0f;
static C2D_TextBuf textBuf;
static C2D_Text    levelText;

static Player player;
static float scrollX = 0;
static int levelWidth = LEVEL1_WIDTH;
static int restartCooldown = 0;


static C2D_TextBuf counterBuf;
static C2D_Text    counterText;
static int lastCount = -1;

void level_init(void)
{
	sheets[0] = C2D_SpriteSheetLoad(LEVEL1_0_IMG);
	sheets[1] = C2D_SpriteSheetLoad(LEVEL1_1_IMG);
	sheets[2] = C2D_SpriteSheetLoad(LEVEL1_2_IMG);

	for (int i = 0; i < 3; i++)
	{
		if (!sheets[i])
		{
			// Release the panels that did load before stopping, so the panic
			// cannot leak them.
			level_exit();
			svcBreak(USERBREAK_PANIC);
			return;
		}
		images[i] = C2D_SpriteSheetGetImage(sheets[i], 0);
	}

	bottomBgSheet = C2D_SpriteSheetLoad(BOTTOM_BG_IMG);
	cloudsSheet = C2D_SpriteSheetLoad(CLOUDS_IMG);
	if (cloudsSheet)
		cloudsImage = C2D_SpriteSheetGetImage(cloudsSheet, 0);
	miniMayroSheet = C2D_SpriteSheetLoad("romfs:/gfx/player_walk_1.t3x");
	enemySheet = C2D_SpriteSheetLoad("romfs:/gfx/gumba.t3x");
	if (enemySheet)
		enemyImage = C2D_SpriteSheetGetImage(enemySheet, 0);
	if (miniMayroSheet)
		miniMayroImage = C2D_SpriteSheetGetImage(miniMayroSheet, 0);
	if (bottomBgSheet)
		bottomBgImage = C2D_SpriteSheetGetImage(bottomBgSheet, 0);

	lastCount = -1;
	textBuf = C2D_TextBufNew(32);
	counterBuf = C2D_TextBufNew(16);
	if (!textBuf || !counterBuf)
	{
		// Out of memory. Release everything loaded so far so the panic below
		// cannot leak, then stop instead of dereferencing NULL every frame.
		level_exit();
		svcBreak(USERBREAK_PANIC);
		return;
	}
	if (defaultFont)
		C2D_TextFontParse(&levelText, defaultFont, textBuf, "LEVUL 1");
	else
		C2D_TextParse(&levelText, textBuf, "LEVUL 1");
	C2D_TextOptimize(&levelText);

	// Spawn at the beginning, a little above the first platform so he falls.
	player_set_level(level1_platforms, LEVEL1_NUM_PLATFORMS,
	                   level1_jumpPads, LEVEL1_NUM_JUMPPADS, LEVEL1_WIDTH);
	player_init(&player, 80.0f, 20.0f);
	scrollX = 0;
	restartCooldown = 30;
	audio_init("romfs:/audio/level1.ogg");
}

int level_update(u32 kDown)
{
	u32 kHeld = hidKeysHeld();
	if (player_update(&player, kDown, kHeld))
		g_deathCount++;  // fell off screen

	// Enemy patrol.
	enemyX += enemyVX;
	if (enemyX < enemyLeft) { enemyX = enemyLeft; enemyVX = 3.0f; }
	if (enemyX > enemyRight - 60) { enemyX = enemyRight - 60; enemyVX = -3.0f; }

	// Touch enemy -> death.
	// Gumba collision box: 40x30 centered in the 60x43 sprite.
	if (player.x + PLAYER_W > enemyX + 10 && player.x < enemyX + 50 &&
	    player.y + PLAYER_HEIGHT > enemyY + 8 && player.y < enemyY + 38)
	{
		g_deathCount++;
		return 2;
	}

	// Advance clouds (slow scroll right, wrapping).
	cloudOffset += 0.3f;
	if (cloudOffset >= 600.0f) cloudOffset -= 600.0f;

	// Camera with a dead zone in the middle: only moves when the player
	// leaves the central band, so small movements don't scroll the view.
	float playerCenter = player.x + PLAYER_W / 2.0f - scrollX;
	float deadLeft  = SCREEN_TOP_W / 2.0f - 50.0f;
	float deadRight = SCREEN_TOP_W / 2.0f + 50.0f;
	if (playerCenter < deadLeft)
		scrollX -= (deadLeft - playerCenter);
	else if (playerCenter > deadRight)
		scrollX += (playerCenter - deadRight);
	if (scrollX < 0) scrollX = 0;
	if (scrollX > levelWidth - SCREEN_TOP_W) scrollX = levelWidth - SCREEN_TOP_W;

	// Touch a goal -> next level.
	for (int i = 0; i < LEVEL1_NUM_GOALS; i++)
	{
		const Rect *g = &level1_goals[i];
		if (player.x + PLAYER_W > g->x0 && player.x < g->x1 &&
		    player.y + PLAYER_HEIGHT > g->y0 && player.y < g->y1)
			return 3;
	}

	if (restartCooldown > 0)
	{
		restartCooldown--;
		return 0;
	}
	if (kDown & KEY_START)
	{
		g_deathCount++;
		restartCooldown = 30;
		return 1; // restart level
	}
	return 0;
}

void level_restart(void)
{
	// Reset player, camera, enemy and cloud — no reallocation.
	lastCount = -1;
	player_reset(&player, 80.0f, 20.0f);
	scrollX = 0;
	restartCooldown = 30;
	enemyX = 1658.0f;
	enemyVX = 3.0f;
	cloudOffset = 0;
	audio_restart();
}

void level_draw(void)
{
	const u32 black = C2D_Color32(0, 0, 0, 255);
	const u32 sky   = C2D_Color32(0x25, 0x96, 0xbe, 255);

	// Top screen: level panels scrolled, then player.
	C2D_TargetClear(screenTop, sky);
	C2D_SceneBegin(screenTop);

	// Scrolling clouds (two copies for seamless wrap).
	if (cloudsImage.subtex)
	{
		C2D_DrawImageAt(cloudsImage, cloudOffset, 0, 0, NULL, 1.0f, 1.0f);
		C2D_DrawImageAt(cloudsImage, cloudOffset - 600.0f, 0, 0, NULL, 1.0f, 1.0f);
	}

	// Panel world X positions: 0, 692, 1384.
	int panelX[3] = { 0, 692, 1384 };
	for (int i = 0; i < 3; i++)
	{
		if (!images[i].subtex)
			continue;
		float sx = panelX[i] - scrollX;
		// Only draw if intersects screen.
		if (sx + images[i].subtex->width > 0 && sx < SCREEN_TOP_W)
			C2D_DrawImageAt(images[i], sx, 0, 0, NULL, 1.0f, 1.0f);
	}

	// Draw jump pad (gray block) so it's visible, before player (player on top).
	C2D_DrawRectSolid(259.0f - scrollX, 136.0f, 0, 52.0f, 9.0f, C2D_Color32(128, 128, 128, 255));
	C2D_DrawRectSolid(1433.0f - scrollX, 147.0f, 0, 52.0f, 9.0f, C2D_Color32(128, 128, 128, 255));

	player_draw(&player, scrollX);

	// Enemy.
	if (enemyImage.subtex)
		C2D_DrawImageAt(enemyImage, enemyX - scrollX, enemyY, 0, NULL, 1.0f, 1.0f);

	// Bottom screen: in-game background.
	C2D_TargetClear(screenBottom, black);
	C2D_SceneBegin(screenBottom);
	if (bottomBgImage.subtex)
		C2D_DrawImageAt(bottomBgImage, 0, 0, 0, NULL, 1.0f, 1.0f);
	// Progress bar (pill) at y~59, x~20..306; mini Mayro follows progress.
	if (miniMayroImage.subtex)
	{
		float progress = player.x / (float)(levelWidth - PLAYER_W);
		if (progress < 0) progress = 0;
		if (progress > 1) progress = 1;
		// Pill center line at y~62; mini Mayro centered on it.
		float pillLeft = 20.0f, pillRight = 306.0f, pillY = 62.0f;
		float mx = pillLeft + progress * (pillRight - pillLeft);
		float miniH = 32.0f;
		float mscale = miniH / 78.0f;
		C2D_DrawImageAt(miniMayroImage, mx - (46.0f*mscale)/2.0f, pillY - 8.0f, 0, NULL, mscale, mscale);
	}
	const float sx = 2.0f, sy = 2.0f;
	C2D_DrawText(&levelText, C2D_WithColor, 4.0f, 4.0f, 0.0f, sx, sy, C2D_Color32(0, 0, 0, 255));
	// "X LIF" counter in top-right of bottom screen.
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

void level_exit(void)
{
	for (int i = 0; i < 3; i++)
	{
		if (sheets[i])
		{
			C2D_SpriteSheetFree(sheets[i]);
			sheets[i] = NULL;
		}
	}
	if (textBuf)
	{
		C2D_TextBufDelete(textBuf);
		textBuf = NULL;
	}
	if (counterBuf)
	{
		C2D_TextBufDelete(counterBuf);
		counterBuf = NULL;
	}
	if (cloudsSheet)
	{
		C2D_SpriteSheetFree(cloudsSheet);
		cloudsSheet = NULL;
	}
	if (miniMayroSheet)
	{
		C2D_SpriteSheetFree(miniMayroSheet);
		miniMayroSheet = NULL;
	}
	if (enemySheet)
	{
		C2D_SpriteSheetFree(enemySheet);
		enemySheet = NULL;
	}
	if (bottomBgSheet)
	{
		C2D_SpriteSheetFree(bottomBgSheet);
		bottomBgSheet = NULL;
	}
	audio_exit();
	player_exit();
}
