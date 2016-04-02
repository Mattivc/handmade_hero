/*
	TODO(Matias): This is not a final platform layer!

	- Save game locaiton
	- Getting a handle to our own executable file
	- Asset loading path
	- Threading (launch a thread)
	- Raw Input (support for multiple keybard)
	- Sleep/timeBeginPeriod
	- ClipCursor (for multi monitor support)
	- Fullscreen support
	- WM_SETCURSOR (control cursor visibility)
	- QuaerCancelAutoplay
	- WM_ACTIVATEAPP (for when we are not the active applicaiton)
	- Blit speed improvements
	- Hardware acceleration
	- GetKeyboardLayouts

	Just a partial list!
*/


#define internal static
#define local_persist static
#define global static

#define PI_32 3.14159265359f

#include <stdint.h>
#include <math.h>

#include "handmade.cpp"

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"


#define BYTES_PER_PIXEL 4

global bool GlobalRunning;
global win32_offscreen_buffer GlobalBackbuffer;
global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void Win32LoadXInput(void) {
	// TODO(Matias): Test on WIndows 8

	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

    if(!XInputLibrary)
    {
        // TODO(casey): Diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

	if(XInputLibrary) {
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState) { XInputSetState = XInputSetStateStub; }

		// TODO(Matias): Diagnostic
	} else {
		// TODO(Matias): Diagnostic
	}
}

internal void Win32InitSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
	// NOTE(Matias): Load the library
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

	if (DSoundLibrary)
	{
		// NOTE(Matias): Get a DirectSound object
		direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		// TODO(Matias): Double-check that this works on Win XP - DirectSound 7 or 8
		LPDIRECTSOUND DirectSound;

		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{

			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				
				// NOTE(Matias): Create a primary buffer
				// TODO(Matias): Use global focus?
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					
					if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						// NOTE(Matias): We have set the format of the primary buffer
						OutputDebugStringA("Primary buffer was set\n");
					}
					else
					{
						// TODO(Matias): Diagnostic
					}
				}
				else
				{
					// TODO(Matias): Diagnostic	
				}
			}
			else
			{
				// TODO(Matias): Diagnostic
			}
			
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			// NOTE(Matias): Create a secondary buffer
			
			if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
			{
				OutputDebugStringA("Secondary buffer was set\n");
			}
			
			// NOTE(Matias): Start playing
		}
		else
		{
			// TODO(Matias): Diagnostic
		}
	}
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Dimension;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Dimension.Heigth = ClientRect.bottom - ClientRect.top;
	Dimension.Width = ClientRect.right - ClientRect.left;

	return Dimension;
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Heigth)
{

	// TODO(Matias): Bulletproof this, free after, then free first if that fail
	// TODO(Matias): Free our DIBSection

	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Heigth;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	
	
	int BitmapMemorySize = BYTES_PER_PIXEL*Width*Heigth;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	// TODO(Matias): Clear to black

	Buffer->Pitch = Width*BYTES_PER_PIXEL;
}

internal void Win32DisplayBufferToWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{

	// TODO(Matias): Aspect ratio correction
	// TODO(Matias): Play with strech modes
	StretchDIBits(DeviceContext,
		          0, 0, WindowWidth, WindowHeight,
		          0, 0, Buffer->Width, Buffer->Height,
		          Buffer->Memory,
		          &Buffer->Info,
		          DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK Win32MainWindowCallback (HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;

	switch(Message)
	{

		case WM_CLOSE:
		{
			// TODO(Matias): Handle this with a message to the user
			GlobalRunning = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY:
		{
			// TODO(Matias): Handle this as an error - recreate window
			GlobalRunning = false;
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32_t VKCode = wParam;
			bool KeyDown = (lParam & (1 << 30)) != 0;
			bool KeyUp = (lParam & (1 << 31)) == 0;

			if (KeyDown != KeyUp) {
				if (VKCode == 'W') {
					
				} else if (VKCode == 'A') {
					
				} else if (VKCode == 'S') {
					
				} else if (VKCode == 'D') {
					
				} else if (VKCode == 'Q') {
					
				} else if (VKCode == 'E') {
					
				} else if (VKCode == VK_UP) {
					
				} else if (VKCode == VK_LEFT) {
					
				} else if (VKCode == VK_DOWN) {
					
				} else if (VKCode == VK_RIGHT) {
					
				} else if (VKCode == VK_ESCAPE) {

				} else if (VKCode == VK_SPACE) {
					
				}
			}

			// Handle Alt+F4
			bool AltKeyDown = (lParam & (1 << 29)) != 0;
			if (VKCode == VK_F4 && AltKeyDown)
			{
				GlobalRunning = false;
			}

		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferToWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Heigth);

			EndPaint(Window, &Paint);
		} break;

		default:
		{
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
	}

	return Result;
}


internal void Win32ClearBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8_t *DestSample = (uint8_t *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (uint8_t *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
	}

	GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

internal void Win32FillSoundBuffer (win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
	                                game_sound_output_buffer *SourceBuffer)
{
    // TODO(casey): More strenuous test!
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
    {
        // TODO(casey): assert that Region1Size/Region2Size is valid

        // TODO(casey): Collapse these two loops
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16_t *DestSample = (int16_t *)Region1;
        int16_t *SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16_t *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
	NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal int CALLBACK WinMain (HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
  	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	int64_t PerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, 0, 0, Instance, 0);

		if(Window)
		{
			HDC DeviceContext = GetDC(Window);

			int XOffset = 0;
			int YOffset = 0;

			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16_t)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

			Win32InitSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);



			#if HANDMADE_INTERNAL
				LPVOID BaseAddress = (LPVOID)Terabytes((uint64_t)2);
			#else
				LPVOID BaseAddress = 0;
			#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes((uint64_t)4);

			uint64_t TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

			GameMemory.TransientStorage = ((uint8_t *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);


			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);

				uint64_t LastCycleCount = __rdtsc();

				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				while(GlobalRunning)
				{
					MSG Message;


					while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
					{
						if(Message.message == WM_QUIT)
						{
							GlobalRunning = false;
						}

						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}

					// TODO(Matias): Should we poll this more fequently?
					int MaxControllerCount = XUSER_MAX_COUNT;
					if (MaxControllerCount > ArrayCount(NewInput->Controllers)) {
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}

					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++)
					{

						game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							// TODO(Matias): Se if packet number increments to rapidly
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->IsAnalog = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;

							float X = (float)Pad->sThumbLX / 32768.0f;
							float Y = (float)Pad->sThumbLY / 32768.0f;

							NewController->MinX = NewController->MaxX = NewController->EndX = X;
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;

							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A, &NewController->Down);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B, &NewController->Right);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X, &NewController->Left);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y, &NewController->Up);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

							//bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
							//bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

						}
						else
						{
							// NOTE(Matias): The controller is not available.
						}
					}

					DWORD BytesToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0;
					DWORD PlayCursor = 0;
					DWORD WriteCursor = 0;
					bool SoundIsValid = false;
					if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
					{
						BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
						TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
						// TODO(Matias): We need a more accurate check than ByteToLock == PlayCursor
						if (BytesToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - BytesToLock;
						}

						SoundIsValid = true;
					}

					

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackbuffer.Memory;
					Buffer.Width = GlobalBackbuffer.Width;
					Buffer.Height = GlobalBackbuffer.Height;
					Buffer.Pitch = GlobalBackbuffer.Pitch;

					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

					// NOTE(Matias): DirectSound output test
					if(SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
					}

					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
					Win32DisplayBufferToWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Heigth);

					uint64_t EndCycleCount = __rdtsc();

					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);

					uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
					int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					int32_t MSPerFrame = (int32_t)((1000*CounterElapsed) / PerfCounterFrequency);

					int32_t FPS = PerfCounterFrequency / CounterElapsed;
					int32_t MCPF = (int32_t)(CyclesElapsed / (1000 * 1000));
	#if 0
					char Buffer[256];
					wsprintf(Buffer, "Frame: %dms - %d FPS - %d MCycles\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(Buffer);
	#endif
					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
				}
			}
			else
			{
				// TODO(Matias): Logging
			}
		}
		else
		{
			// TODO(Matias): Logging
		}

	}
	else
	{
		// TODO(Matias): Logging
	}

	return 0;
}
