#ifndef AUDIO_H
#define AUDIO_H

#include <3ds.h>

// Start playing an Ogg Vorbis file from romfs (e.g. "romfs:/audio/level1.ogg").
// Returns true on success.
bool audio_init(const char* path);

// Stop playback and free resources.
void audio_exit(void);
void audio_update(void);
void audio_restart(void);

#endif // AUDIO_H
