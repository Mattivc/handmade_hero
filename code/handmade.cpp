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

internal void GameUpdateAndRender(game_input *Input,
	                              game_offscreen_buffer *Buffer,
	                              game_sound_output_buffer *SoundBuffer)
{
	local_persist int BlueOffset = 0;
	local_persist int GreenOffset = 0;
	local_persist int ToneHz = 256;

	game_controller_input *Input0 = &Input->Controllers[0];

	if (Input0->IsAnalog)
	{
		ToneHz = 256 + (int)(128.0f*(Input0->EndY));
		BlueOffset += (int)4.0f*(Input0->EndX);
	}
	else
	{

	}

	if (Input0->Down.EndedDown) {
		GreenOffset += 1;
	}


	// TODO(Matias): Allow sample offsets here for more robust platform options
	GameOutputSound(SoundBuffer, ToneHz);
	RenderWierdGradient(Buffer, BlueOffset, GreenOffset);
}
