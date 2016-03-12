0#ifndef HANDMADE_H

#define ArrayCount(Array) sizeof(Array) / sizeof((Array)[0])

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

struct game_button_state
{
	int HalfTransitionCount;
	bool EndedDown;
};

struct game_controller_input
{
	bool IsAnalog;

	float StartX;
	float StartY;

	float MinX;
	float MinY;

	float MaxX;
	float MaxY;

	float EndX;
	float EndY;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;			
		};
	};
};

struct game_input
{
	game_controller_input Controllers[4];
};

// Take in: Timing, Controller/Keyboard input, bitmat buffer to render to, sound buffer to write to.
internal void GameUpdateAndRender(game_input *Input,
	                              game_offscreen_buffer *Buffer,
	                              game_sound_output_buffer *SoundBuffer);

#define HANDMADE_H value
#endif
