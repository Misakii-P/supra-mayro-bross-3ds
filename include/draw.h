// Small C-friendly wrappers over citro2d.
//
// citro2d's inline helpers use C++ default arguments, which the C compiler
// cannot expand, so the common "draw an image at x,y" cases are wrapped here
// once instead of at every call site.

#ifndef DRAW_H
#define DRAW_H

#include <citro2d.h>

#define DRAW_DEPTH_DEFAULT     (0.0f)

static inline void draw_image(C2D_Image img, float x, float y)
{
	C2D_DrawImageAt(img, x, y, DRAW_DEPTH_DEFAULT, NULL, 1.0f, 1.0f);
}

#endif // DRAW_H
