#include "audio.h"

#include <tremor/ivorbisfile.h>
#include <tremor/ivorbiscodec.h>
#include <3ds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Number of wave buffers in the ring.
#define NUM_WAVEBUFS (ARRAY_SIZE(s_waveBufs))

// Bytes of audio per wave buffer. Small enough to decode fast in the main loop,
// large enough that the DSP does not starve between refills.
#define BUF_MS 120

// How many times fillBuffer may rewind mid-buffer before giving up. Guards
// against spinning forever on a corrupt or empty stream.
#define MAX_WIND_PER_BUFFER 4

// Bound on the spin used to wait for the DSP to release the wave memory.
#define DSP_DRAIN_SPINS 200000

static ndspWaveBuf s_waveBufs[3];
static int16_t    *s_audioBuffer = NULL;
static u32         s_waveBufNsamples = 0;   // capacity of ONE buffer, in samples

static OggVorbis_File s_vorbisFile;
static FILE          *s_fh = NULL;
static bool           s_playing = false;
static char           s_curPath[64];

//---------------------------------------------------------------------------------
static void setupNdsp(void)
{
	vorbis_info *vi = ov_info(&s_vorbisFile, -1);
	ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(0, vi->rate);
	ndspChnSetFormat(0, vi->channels == 1
		? NDSP_FORMAT_MONO_PCM16
		: NDSP_FORMAT_STEREO_PCM16);
}

//---------------------------------------------------------------------------------
static bool audioInit(void)
{
	vorbis_info *vi = ov_info(&s_vorbisFile, -1);

	ndspChnReset(0);
	ndspChnWaveBufClear(0);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	setupNdsp();

	const size_t SAMPLES_PER_BUF     = vi->rate * BUF_MS / 1000;
	const size_t CHANNELS_PER_SAMPLE = vi->channels;
	const size_t WAVEBUF_SIZE        = SAMPLES_PER_BUF * CHANNELS_PER_SAMPLE * sizeof(s16);
	const size_t bufferSize          = WAVEBUF_SIZE * NUM_WAVEBUFS;

	s_audioBuffer = (int16_t *)linearAlloc(bufferSize);
	if (!s_audioBuffer)
		return false;

	// Clear it: a partially filled buffer must never expose samples left over
	// from whatever track played before.
	memset(s_audioBuffer, 0, bufferSize);

	memset(s_waveBufs, 0, sizeof(s_waveBufs));
	int16_t *buffer = s_audioBuffer;
	for (size_t i = 0; i < NUM_WAVEBUFS; ++i) {
		s_waveBufs[i].data_vaddr = buffer;
		s_waveBufs[i].status     = NDSP_WBUF_DONE;
		buffer += WAVEBUF_SIZE / sizeof(buffer[0]);
	}

	s_waveBufNsamples = (u32)(WAVEBUF_SIZE / sizeof(s16));
	return true;
}

//---------------------------------------------------------------------------------
static void audioExitImpl(void)
{
	ndspChnReset(0);
	ndspChnWaveBufClear(0);

	// The DSP may still be reading the wave memory after the reset. Freeing it
	// underneath the DSP is a use-after-free, and shows up as intermittent
	// crashes on scene changes and restarts. Wait (bounded) until the channel
	// is idle before handing the memory back.
	for (int i = 0; i < DSP_DRAIN_SPINS; ++i) {
		if (!ndspChnIsPlaying(0))
			break;
	}

	if (s_audioBuffer) {
		linearFree(s_audioBuffer);
		s_audioBuffer = NULL;
	}
	s_waveBufNsamples = 0;
}

//---------------------------------------------------------------------------------
// Decode up to `capacity` samples into `waveBuf_` and queue it.
//
// `capacity` is passed in rather than read from waveBuf_->nsamples: that field is
// overwritten with the number of samples actually decoded, so using it as the
// fill target makes the buffers shrink a little on every refill until playback
// collapses into a stutter.
//
// End of stream is handled by rewinding and continuing to decode into the SAME
// buffer, so a loop point has no gap - the tail of one pass and the head of the
// next are handed to the DSP as one contiguous block.
//
// Returns false only when not a single sample could be produced.
//---------------------------------------------------------------------------------
static bool fillBuffer(ndspWaveBuf *waveBuf_, u32 capacity)
{
	int   totalBytes   = 0;
	int   winds        = 0;
	const int capacityBytes = (int)capacity * (int)sizeof(s16);

	while (totalBytes < capacityBytes)
	{
		const long bytesRead = ov_read(&s_vorbisFile,
		                               (char *)waveBuf_->data_pcm16 + totalBytes,
		                               capacityBytes - totalBytes,
		                               NULL);

		if (bytesRead > 0) {
			totalBytes += (int)bytesRead;
			continue;
		}

		if (bytesRead < 0) {
			// Hard decode error - stop rather than spin.
			break;
		}

		// 0 == end of stream. Rewind and keep decoding into this buffer.
		if (winds >= MAX_WIND_PER_BUFFER)
			break;
		winds++;
		if (ov_pcm_seek(&s_vorbisFile, 0) != 0)
			break;
	}

	if (totalBytes == 0)
		return false;

	// Flush before queueing: once the buffer is on the DSP's list it may start
	// reading immediately, and unflushed cache lines come out as garbage.
	DSP_FlushDataCache(waveBuf_->data_pcm16, (u32)totalBytes);
	waveBuf_->nsamples = (u32)totalBytes / (u32)sizeof(s16);
	ndspChnWaveBufAdd(0, waveBuf_);
	return true;
}

//---------------------------------------------------------------------------------
bool audio_init(const char* path)
{
	audio_exit(); // stop any previous

	s_fh = fopen(path, "rb");
	if (!s_fh) {
		s_curPath[0] = '\0';   // don't let restart resurrect a missing track
		return false;
	}

	int error = ov_open(s_fh, &s_vorbisFile, NULL, 0);
	if (error) {
		fclose(s_fh);
		s_fh = NULL;
		s_curPath[0] = '\0';
		return false;
	}

	if (!audioInit()) {
		ov_clear(&s_vorbisFile);   // closes s_fh for us
		s_fh = NULL;
		s_curPath[0] = '\0';
		return false;
	}

	snprintf(s_curPath, sizeof(s_curPath), "%s", path);

	// Prime the pipeline. A short track may not fill every buffer; that is fine,
	// audio_update tops the ring up as slots free up.
	for (size_t i = 0; i < NUM_WAVEBUFS; ++i) {
		if (!fillBuffer(&s_waveBufs[i], s_waveBufNsamples))
			break;
	}

	s_playing = true;
	return true;
}

//---------------------------------------------------------------------------------
void audio_exit(void)
{
	if (!s_playing)
		return;

	ndspChnReset(0);
	audioExitImpl();
	ov_clear(&s_vorbisFile);   // closes s_fh for us
	s_fh = NULL;
	s_playing = false;
}

//---------------------------------------------------------------------------------
void audio_update(void)
{
	if (!s_playing)
		return;

	for (size_t i = 0; i < NUM_WAVEBUFS; ++i) {
		if (s_waveBufs[i].status != NDSP_WBUF_DONE)
			continue;
		// fillBuffer rewinds internally, so loops are handled inside it.
		if (!fillBuffer(&s_waveBufs[i], s_waveBufNsamples)) {
			audio_exit();
			return;
		}
	}
}

//---------------------------------------------------------------------------------
void audio_restart(void)
{
	if (s_curPath[0] == '\0')
		return;

	// Clean teardown + re-init. Rewinding a stream the decoder is mid-way
	// through, or reusing wave buffers the DSP still owns, both misbehave; a
	// full re-init is cheap now that decoding is synchronous.
	char path[64];
	snprintf(path, sizeof(path), "%s", s_curPath);
	audio_exit();
	audio_init(path);
}