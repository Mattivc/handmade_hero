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

inline uint32_t SafeTruncateUInt64(uint64_t Value)
{
	Assert(Value <= 0xFFFFFFFF);
	return (uint32_t)Value;
}

/*
	TODO(Matias): Services that the platform layaer profivides to the game.
*/

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
	uint32_t ContentSize;
	void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory);
#endif

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
	bool IsConnected;
	bool IsAnalog;
	float StickAverageX;
	float StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;		
			game_button_state Back;
		};
	};
};

struct game_input
{
	// TODO(Matias): Insert clock values here
	game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	return &(Input->Controllers[ControllerIndex]);
}

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
