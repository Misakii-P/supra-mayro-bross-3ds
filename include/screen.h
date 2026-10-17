// Shared screen render targets.
//
// These are created ONCE at startup and reused by every scene. Creating them
// per scene leaks ~360 KB of VRAM each time (the 3DS only has 6 MB of VRAM,
// shared between framebuffers and textures), so after a dozen scene
// transitions vramAlloc fails, C2D_CreateScreenTarget returns NULL, and
// C2D_TargetClear dereferences it.
//
// devkitPro also explicitly warns against creating/destroying render targets
// inside the render loop, because the GPU runs asynchronously and may still be
// reading a target that has just been freed.

#ifndef SCREEN_H
#define SCREEN_H

#include <citro2d.h>

extern C3D_RenderTarget *screenTop;
extern C3D_RenderTarget *screenBottom;

// Create both targets. Call once, before any scene_init.
bool screen_init(void);

#endif // SCREEN_H