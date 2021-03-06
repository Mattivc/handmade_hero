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
#include <stdio.h>
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
global int64_t GlobalPerfCounterFrequency;

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

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32_t FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					Result.ContentSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{

			}
		}	
		else
		{

		}
		CloseHandle(FileHandle);
	}
	else
	{

	}

	return Result;
}

internal void DEBUGPlatformFreeFileMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory)
{
	bool Result = 0;

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			
		}

		CloseHandle(FileHandle);
	}
	else
	{

	}

	return Result;
}

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
			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
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
	Buffer->Pitch = Width*BYTES_PER_PIXEL;
	Buffer->BytesPerPixel = BYTES_PER_PIXEL;
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
			Assert(!"Keyboard input came in trough a non-dispatched message.");
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

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool IsDown)
{
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown;
	NewState->HalfTransitionCount++;
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
	NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal float Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
	// TODO(Matias): Make this magnitude based
	float Result = 0;
	if ((Value > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) || (Value < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
	{
		Result = (float)(Value / 32768.0f);
	}

	return Result;
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
	MSG Message;

	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			} break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32_t VKCode = (uint32_t)Message.wParam;
				bool WasDown = (Message.lParam & (1 << 30)) != 0;
				bool IsDown = (Message.lParam & (1 << 31)) == 0;
				if (WasDown != IsDown)
				{
					if (VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
					}
					else if (VKCode == 'A')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if (VKCode == 'S')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if (VKCode == 'D')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if (VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if (VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
					}
					else if (VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
					}
				}

				// Handle Alt+F4
				bool AltKeyDown = (Message.lParam & (1 << 29)) != 0;
				if (VKCode == VK_F4 && AltKeyDown)
				{
					GlobalRunning = false;
				}
			} break;
			default:
			{
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}
		}
	}
}

inline LARGE_INTEGER Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	return (float)(End.QuadPart - Start.QuadPart) / (float)GlobalPerfCounterFrequency;
}

inline void Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer, int X, int Top, int Bottom, uint32_t Color)
{
	uint8_t *Pixel = ((uint8_t *)Backbuffer->Memory + X*Backbuffer->BytesPerPixel + Top*Backbuffer->Pitch);
	for (int Y = Top; Y < Bottom; ++Y)
	{
		*(uint32_t *)Pixel = Color;
		Pixel += Backbuffer->Pitch;
	}
}

internal void Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer, win32_sound_output *SoundOutput, float C, int PadX, int Top, int Bottom, DWORD Value, uint32_t Color)
{
	Assert(Value < SoundOutput->SecondaryBufferSize);
	int X = PadX + (int)(C * (float)Value);
	Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

internal void Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer, int MarkerCount, win32_debug_time_marker *Markers, win32_sound_output *SoundOutput, float TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int Top = PadY;
	int Bottom = Backbuffer->Height - PadY;

	float C = (float)Backbuffer->Width / (float)SoundOutput->SecondaryBufferSize;
	for (int PlayCursorIndex = 0; PlayCursorIndex < MarkerCount; ++PlayCursorIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[PlayCursorIndex];
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFFFF0000);
		
	}
}

extern int CALLBACK WinMain (HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
  	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	GlobalPerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

	// NOTE(Matias): Set the Windows scheduler granularity to 1 MS, so that our sleep can be more granular
	UINT DesiredSchedulerMS = 1;
	bool SleepIsGranular = timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR;

	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	// TODO(Matias): How do we realiably query this on windows?
	#define MonitorRefreshHz 60
	#define GameUpdateHz (MonitorRefreshHz / 2)
	#define FramesOfAudioLatency 3
	float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT, 0, 0, Instance, 0);

		if(Window)
		{
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16_t)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = FramesOfAudioLatency*(SoundOutput.SamplesPerSecond / GameUpdateHz);

			Win32InitSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);


			GlobalRunning = true;
#if 0
			while(GlobalRunning)
			{
				DWORD PlayCursor;
				DWORD WriteCursor;
				GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

				char Buffer[256];
				snprintf(Buffer, sizeof(Buffer), "PC:%u WC:%u\n", PlayCursor, WriteCursor);
				OutputDebugStringA(Buffer);
			}
#endif
			

			int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);



			#if HANDMADE_INTERNAL
				LPVOID BaseAddress = (LPVOID)Terabytes((uint64_t)2);
			#else
				LPVOID BaseAddress = 0;
			#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);

			uint64_t TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

			GameMemory.TransientStorage = ((uint8_t *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);


			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				uint64_t LastCycleCount = __rdtsc();

				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				int DebugTimeMarkersIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[GameUpdateHz/2] = {};

				DWORD LastPlayCursor = 0;
				bool SoundIsValid = false;

				while(GlobalRunning)
				{
					game_controller_input *OldKeyBoardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;

					for (int ButtonIndex=0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyBoardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(NewKeyboardController);

					// TODO(Matias): Should we poll this more fequently?
					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					if (MaxControllerCount > ArrayCount(NewInput->Controllers) - 1) {
						MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
					}

					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++)
					{
						int OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
						game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							NewController->IsConnected = true;

							// TODO(Matias): Se if packet number increments to rapidly
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							NewController->IsAnalog = true;
							NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0)
							{
								NewController->StickAverageY = 1.0f;
								NewController->IsAnalog = false;
							}
							if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0)
							{
								NewController->StickAverageY = -1.0f;
								NewController->IsAnalog = false;
							}
							if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0)
							{
								NewController->StickAverageX = -1.0f;
								NewController->IsAnalog = false;
							}
							if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0)
							{
								NewController->StickAverageX = 1.0f;
								NewController->IsAnalog = false;
							}

							float Threshold = 0.5;
							Win32ProcessXInputDigitalButton(((NewController->StickAverageX < -Threshold) ? 1 : 0), &OldController->MoveLeft, 1, &NewController->MoveLeft);
							Win32ProcessXInputDigitalButton(((NewController->StickAverageX > Threshold) ? 1 : 0), &OldController->MoveRight, 1, &NewController->MoveRight);
							Win32ProcessXInputDigitalButton(((NewController->StickAverageY < -Threshold) ? 1 : 0), &OldController->MoveDown, 1, &NewController->MoveDown);
							Win32ProcessXInputDigitalButton(((NewController->StickAverageY > Threshold) ? 1 : 0), &OldController->MoveUp, 1, &NewController->MoveUp);


							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_BACK, &NewController->Back);
						}
						else
						{
							// NOTE(Matias): The controller is not available.
							NewController->IsConnected = false;
						}
					}

					DWORD BytesToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0;
					
					if (SoundIsValid)
					{


						BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
						TargetCursor = ((LastPlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
						if (BytesToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - BytesToLock;
						}
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
#if HANDMADE_INTERNAL
						DWORD PlayCursor;
						DWORD WriteCursor;
						GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
						char DebugBuffer[256];
						snprintf(DebugBuffer, sizeof(DebugBuffer), "LPC:%u BTL:%u TC:%u BTW:%u - PC:%u WC:%u\n",
							LastPlayCursor, BytesToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor);
						OutputDebugStringA(DebugBuffer);
#endif
						Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
					}

					LARGE_INTEGER WorkCounter = Win32GetWallClock();
					float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);


					float SecondsElapsedForFrame = WorkSecondsElapsed;
					if (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						if (SleepIsGranular)
						{
							DWORD SleepMS = (DWORD)(1000.0f*(TargetSecondsPerFrame - SecondsElapsedForFrame));
							if (SleepMS > 0)
							{
								Sleep(SleepMS);
							}
						}
						while (SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
						}
					}
					else
					{
						// TODO(Matias): Missed frame rate
						// TODO(Matias): Logging
					}

					LARGE_INTEGER EndCounter = Win32GetWallClock();
					float MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);                    
					LastCounter = EndCounter;

					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
#if HANDMADE_INTERNAL
					Win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, &SoundOutput, TargetSecondsPerFrame);
#endif
					Win32DisplayBufferToWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Heigth);

					DWORD PlayCursor;
					DWORD WriteCursor;

					if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
					{
						LastPlayCursor = PlayCursor;
						if (!SoundIsValid)
						{
							SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
							SoundIsValid = true;
						}
						SoundIsValid = true;
					}
					else
					{
						SoundIsValid = false;
					}
#if HANDMADE_INTERNAL
					{
						win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkersIndex++];

						if (DebugTimeMarkersIndex > ArrayCount(DebugTimeMarkers))
						{
							DebugTimeMarkersIndex = 0;
						}
						Marker->PlayCursor = PlayCursor;
						Marker->WriteCursor = WriteCursor;
					}
#endif
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
					
					

					uint64_t EndCycleCount = __rdtsc();
					uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
					LastCycleCount = EndCycleCount;
					
					double FPS = 0.0f;
					double MCPF = ((double)CyclesElapsed / (1000.0f * 1000.0f));

					char FPSBuffer[256];
					snprintf(FPSBuffer, sizeof(FPSBuffer), "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
					OutputDebugStringA(FPSBuffer);
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
