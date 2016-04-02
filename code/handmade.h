#ifndef HANDMADE_H

/*
	HANDMADE_INTERNAL:
		0 - Build for public release
		1 - Build for developer only

	HANDMADE_SLOW:
		0 - No slow code allowed
		1 - Slow code allowed
*/

#if HANDMADE_SLOW
	#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
	#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

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
	// TODO(Matias): Insert clock values here
	game_controller_input Controllers[4];
};

struct game_memory
{
	bool IsInitialized;
	uint64_t PermanentStorageSize;
	void *PermanentStorage; // NOTE(Matias): REQUIRED to be cleared to zero at startup

	uint64_t TransientStorageSize;
	void *TransientStorage;  // NOTE(Matias): REQUIRED to be cleared to zero at startup
};

// Take in: Timing, Controller/Keyboard input, bitmat buffer to render to, sound buffer to write to.
internal void GameUpdateAndRender(game_memory *Memory, game_input *Input,
	                              game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

struct game_state
{
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};

#define HANDMADE_H value
#endif
