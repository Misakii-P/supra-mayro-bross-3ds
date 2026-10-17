// Title screen: logo on the top screen, tank/PLAY artwork on the touch screen.

#include "title.h"
#include "draw.h"
#include "screen.h"

#include <citro2d.h>

#define TITLE_TOP_IMG     "romfs:/gfx/titletop.t3x"
#define TITLE_BOTTOM_IMG  "romfs:/gfx/titlebottom.t3x"

static C2D_SpriteSheet topSheet;
static C2D_SpriteSheet bottomSheet;

static C2D_Image topImage;
static C2D_Image bottomImage;

void title_init(void)
{
	topSheet    = C2D_SpriteSheetLoad(TITLE_TOP_IMG);
	bottomSheet = C2D_SpriteSheetLoad(TITLE_BOTTOM_IMG);

	if (!topSheet || !bottomSheet)
	{
		// Release whichever one did load before stopping, otherwise the panic
		// below leaks a sheet.
		title_exit();
		svcBreak(USERBREAK_PANIC);
		return;
	}

	topImage    = C2D_SpriteSheetGetImage(topSheet, 0);
	bottomImage = C2D_SpriteSheetGetImage(bottomSheet, 0);
}

void title_draw(void)
{
	const u32 black = C2D_Color32(0, 0, 0, 255);

	C2D_TargetClear(screenTop, black);
	C2D_SceneBegin(screenTop);
	if (topImage.subtex)
		draw_image(topImage, 0, 0);

	C2D_TargetClear(screenBottom, black);
	C2D_SceneBegin(screenBottom);
	if (bottomImage.subtex)
		draw_image(bottomImage, 0, 0);
}

int title_update(u32 kDown, touchPosition touch)
{
	if (kDown & KEY_START)
		return 1;

	if (kDown & KEY_TOUCH)
	{
		if (touch.px >= 100 && touch.px <= 225 &&
		    touch.py >= 15  && touch.py <= 180)
			return 1;
	}

	return 0;
}

void title_exit(void)
{
	if (topSheet)
	{
		C2D_SpriteSheetFree(topSheet);
		topSheet = NULL;
	}
	if (bottomSheet)
	{
		C2D_SpriteSheetFree(bottomSheet);
		bottomSheet = NULL;
	}
}
