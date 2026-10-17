#include "screen.h"

C3D_RenderTarget *screenTop    = NULL;
C3D_RenderTarget *screenBottom = NULL;

bool screen_init(void)
{
	screenTop    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	screenBottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	return screenTop != NULL && screenBottom != NULL;
}