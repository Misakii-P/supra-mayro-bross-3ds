#ifndef LEVEL_H
#define LEVEL_H

#include <3ds.h>

void level_init(void);
void level_restart(void);
int  level_update(u32 kDown);
void level_draw(void);
void level_exit(void);

#endif // LEVEL_H
