#ifndef HANDMADE_H


/*
	TODO(Matias): Services that the platform layaer profivides tot he game.
*/


/*
	NOTE(Matias): Services that the game provides tot he platform layer.
	(This may expand in the future - sound of seperate thread, etc.)
*/


struct game_offscreen_buffer
{
	// NOTE(Matias): Pixels are always 32-bits wide, Memory order: BB GG RR XX
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16_t *Samples;
};

// Take in: Timing, Controller/Keyboard input, bitmat buffer to render to, sound buffer to write to.
void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset,
	                              game_sound_output_buffer *SoundBuffer, int ToneHz);

#define HANDMADE_H value
#endif
