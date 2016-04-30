#if !defined(WIN32_HANDMADE_H)


struct win32_window_dimension {
	int Width;
	int Heigth;
};

struct win32_offscreen_buffer
{
	// NOTE(Matias): Pixels are always 32-bits wide, Memory order: BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_sound_output {
	int SamplesPerSecond;
	uint32_t RunningSampleIndex;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	float tSine;
	int LatencySampleCount;
};

struct win32_debug_time_marker
{
	DWORD PlayCursor;
	DWORD WriteCursor;
};

#define WIN32_HANDMADE_H
#endif
