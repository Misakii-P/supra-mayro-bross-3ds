#ifndef FONT_H
#define FONT_H

#include <citro2d.h>

// The default font for the game, loaded from romfs:/fonts/FriendsZ-Regular.bcfnt
extern C2D_Font defaultFont;

void font_init(void);
void font_exit(void);

#endif // FONT_H
