#include "font.h"

#include <citro2d.h>
#include <stdio.h>

C2D_Font defaultFont = NULL;

void font_init(void)
{
	defaultFont = C2D_FontLoad("romfs:/fonts/FriendsZ-Regular.bcfnt");
	if (defaultFont)
		C2D_FontSetFilter(defaultFont, GPU_NEAREST, GPU_NEAREST);
}

void font_exit(void)
{
	if (defaultFont)
	{
		C2D_FontFree(defaultFont);
		defaultFont = NULL;
	}
}
