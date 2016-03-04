#include "handmade.h"


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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	RenderWierdGradient(Buffer, BlueOffset, GreenOffset);
}
