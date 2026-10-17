// Level 3: BOSS - a single big Gumba patrolling its zone.

#include "level3.h"
#include "player.h"
#include "draw.h"
#include "game.h"
#include "font.h"
#include "audio.h"
#include "screen.h"

#include <citro2d.h>
#include <stdio.h>

#define BOSS_0_IMG    "romfs:/gfx/boss_0.t3x"
#define BOSS_1_IMG    "romfs:/gfx/boss_1.t3x"
#define BOTTOM_BG_IMG "romfs:/gfx/bottomgame.t3x"

#define BOSS_NUM_PANELS 2

static C2D_SpriteSheet sheets[BOSS_NUM_PANELS];
static C2D_Image       images[BOSS_NUM_PANELS];

static C2D_SpriteSheet bossSheet;
static C2D_Image       bossImage;

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

// Big Gumba: slides along its red zone, floating (no gravity).
static struct { float x; float vx; float y; } boss;
#define BOSS_SPEED 2.0f
#define BOSS_SPAWN_X  90.0f
#define BOSS_SPAWN_Y 100.0f

void level3_init(void)
{
	sheets[0] = C2D_SpriteSheetLoad(BOSS_0_IMG);
	sheets[1] = C2D_SpriteSheetLoad(BOSS_1_IMG);
	for (int i = 0; i < BOSS_NUM_PANELS; i++)
	{
		if (!sheets[i])
		{
			// Release the panels that did load before stopping, so the panic
			// cannot leak them.
			level3_exit();
			svcBreak(USERBREAK_PANIC);
			return;
		}
		images[i] = C2D_SpriteSheetGetImage(sheets[i], 0);
	}

	bossSheet = C2D_SpriteSheetLoad("romfs:/gfx/gumba.t3x");
	if (bossSheet)
		bossImage = C2D_SpriteSheetGetImage(bossSheet, 0);

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
		level3_exit();
		svcBreak(USERBREAK_PANIC);
		return;
	}
	if (defaultFont)
		C2D_TextFontParse(&levelText, defaultFont, textBuf, "LEVUL 3");
	else
		C2D_TextParse(&levelText, textBuf, "LEVUL 3");
	C2D_TextOptimize(&levelText);

	player_set_level(level3_platforms, LEVEL3_NUM_PLATFORMS,
	                 level3_jumpPads, LEVEL3_NUM_JUMPPADS, LEVEL3_WIDTH);
	lastCount = -1;
	player_init(&player, BOSS_SPAWN_X, BOSS_SPAWN_Y);

	boss.x  = LEVEL3_BOSS_X0;
	boss.vx = BOSS_SPEED;
	boss.y  = LEVEL3_BOSS_Y;

	scrollX = 0;
	restartCooldown = 30;
	audio_init("romfs:/audio/boss.ogg");
}

void level3_restart(void)
{
	lastCount = -1;
	player_reset(&player, BOSS_SPAWN_X, BOSS_SPAWN_Y);
	scrollX = 0;
	restartCooldown = 30;
	boss.x  = LEVEL3_BOSS_X0;
	boss.vx = BOSS_SPEED;
	boss.y  = LEVEL3_BOSS_Y;
	audio_restart();
}

int level3_update(u32 kDown)
{
	u32 kHeld = hidKeysHeld();
	if (player_update(&player, kDown, kHeld))
		g_deathCount++;

	// Boss slides across its red zone (no gravity), touch -> death.
	// Collision uses the drawn size of the scaled sprite, and the sprite is kept
	// fully inside the red box.
	float bw = bossImage.subtex ? (float)bossImage.subtex->width  * LEVEL3_BOSS_SCALE : 0.0f;
	float bh = bossImage.subtex ? (float)bossImage.subtex->height * LEVEL3_BOSS_SCALE : 0.0f;
	float bossMaxX = LEVEL3_BOSS_X1 - bw;

	boss.x += boss.vx;
	if (boss.x <= LEVEL3_BOSS_X0) { boss.x = LEVEL3_BOSS_X0; boss.vx =  BOSS_SPEED; }
	if (boss.x >= bossMaxX)      { boss.x = bossMaxX;      boss.vx = -BOSS_SPEED; }

	if (player.x + PLAYER_W > boss.x && player.x < boss.x + bw &&
	    player.y + PLAYER_HEIGHT > boss.y && player.y < boss.y + bh)
	{
		g_deathCount++;
		return 2;
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
	if (scrollX > LEVEL3_WIDTH - SCREEN_TOP_W) scrollX = LEVEL3_WIDTH - SCREEN_TOP_W;

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

void level3_draw(void)
{
	const u32 black = C2D_Color32(0, 0, 0, 255);
	const u32 bg    = C2D_Color32(0x36, 0x36, 0x36, 255);

	C2D_TargetClear(screenTop, bg);
	C2D_SceneBegin(screenTop);

	int panelX[BOSS_NUM_PANELS] = { 0, 1024 };
	for (int i = 0; i < BOSS_NUM_PANELS; i++)
	{
		if (!images[i].subtex)
			continue;
		float sx = panelX[i] - scrollX;
		if (sx + images[i].subtex->width > 0 && sx < SCREEN_TOP_W)
			C2D_DrawImageAt(images[i], sx, 0, 0, NULL, 1.0f, 1.0f);
	}

	// Boss, drawn as a scaled-up Gumba.
	if (bossImage.subtex)
		C2D_DrawImageAt(bossImage, boss.x - scrollX, boss.y, 0, NULL,
		                LEVEL3_BOSS_SCALE, LEVEL3_BOSS_SCALE);

	player_draw(&player, scrollX);

	C2D_TargetClear(screenBottom, black);
	C2D_SceneBegin(screenBottom);
	if (bottomBgImage.subtex)
		C2D_DrawImageAt(bottomBgImage, 0, 0, 0, NULL, 1.0f, 1.0f);
	// Progress bar (pill) at y~62, x~20..306; mini Mayro follows progress.
	if (miniMayroImage.subtex)
	{
		float progress = player.x / (float)(LEVEL3_WIDTH - PLAYER_W);
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

void level3_exit(void)
{
	for (int i = 0; i < BOSS_NUM_PANELS; i++)
		if (sheets[i]) { C2D_SpriteSheetFree(sheets[i]); sheets[i] = NULL; }
	if (textBuf)       { C2D_TextBufDelete(textBuf); textBuf = NULL; }
	if (counterBuf)    { C2D_TextBufDelete(counterBuf); counterBuf = NULL; }
	if (bossSheet)     { C2D_SpriteSheetFree(bossSheet); bossSheet = NULL; }
	if (bottomBgSheet) { C2D_SpriteSheetFree(bottomBgSheet); bottomBgSheet = NULL; }
	if (miniMayroSheet){ C2D_SpriteSheetFree(miniMayroSheet); miniMayroSheet = NULL; }
	audio_exit();
	player_exit();
}