#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	local_persist float tSine;
	int16_t ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

	int16_t *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
	    float SineValue = sinf(tSine);
	    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
	    *SampleOut++ = SampleValue;
	    *SampleOut++ = SampleValue;

	    tSine += 2.0f*PI_32/(float)WavePeriod;
	}
}

internal void RenderWierdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{

	// TODO(Matias): See what the optimizer does
	
	uint8_t *Row = (uint8_t *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = 0; X < Buffer->Width; ++X)
		{
			uint8_t Blue = (X + XOffset);
			uint8_t Green = (Y + YOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Buffer->Pitch;
	}
}


internal game_state *GameStartup(void)
{
	game_state *GameState = new game_state;

	if (GameState)
	{
		GameState->BlueOffset = 0;
		GameState->GreenOffset = 0;
		GameState->ToneHz = 256;
	}

	return GameState;
}

internal void GameShutdown(game_state *GameState)
{
	delete GameState;
}

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input,
	                              game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;
		
		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 256;
		Memory->IsInitialized = true;
	}

	game_controller_input *Input0 = &Input->Controllers[0];

	if (Input0->IsAnalog)
	{
		GameState->ToneHz = 256 + (int)(128.0f*(Input0->EndY));
		GameState->BlueOffset += (int)4.0f*(Input0->EndX);
	}
	else
	{

	}

	if (Input0->Down.EndedDown) {
		GameState->GreenOffset += 1;
	}


	// TODO(Matias): Allow sample offsets here for more robust platform options
	GameOutputSound(SoundBuffer, GameState->ToneHz);
	RenderWierdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
