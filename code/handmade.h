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

// Take in: Timing, Controller/Keyboard input, bitmat buffer to render to, sound buffer to write to.
void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset);

#define HANDMADE_H value
#endif
