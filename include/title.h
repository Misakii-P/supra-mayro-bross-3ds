#ifndef TITLE_H
#define TITLE_H

#include <3ds.h>

// Returns 0=stay, 1=level.
int title_update(u32 kDown, touchPosition touch);
void title_init(void);
void title_draw(void);
void title_exit(void);

#endif // TITLE_H
