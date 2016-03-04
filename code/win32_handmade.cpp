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

#include "handmade.cpp"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

// TODO(Matias): Implement sine ourself
#include <math.h>


#define BYTES_PER_PIXEL 4


struct win32_window_dimension {
	int Width;
	int Heigth;
};

struct win32_offscreen_buffer
{
	// NOTE(Matias): Pixels are always 32-bits wide, Memory order: BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

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

struct win32_sound_output {
	int SamplesPerSecond;
	int ToneHz;
	int ToneVolume;
	uint32_t RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	float tSine;
	int LatencySampleCount;
};

internal void Win32FillSoundBuffer (win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
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
        int16_t *SampleOut = (int16_t *)Region1;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            // TODO(casey): Draw this out for people
            float SineValue = sinf(SoundOutput->tSine);
            int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f*PI_32*1.0f/(float)SoundOutput->WavePeriod;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        SampleOut = (int16_t *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            float SineValue = sinf(SoundOutput->tSine);
            int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f*PI_32*1.0f/(float)SoundOutput->WavePeriod;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
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
			SoundOutput.ToneHz = 256;
			SoundOutput.ToneVolume = 3000;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16_t)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

			Win32InitSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);

			uint64_t LastCycleCount = __rdtsc();

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
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
				{
					XINPUT_STATE ControllerState;

					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// TODO(Matias): Se if packet number increments to rapidly
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16_t StickX = Pad->sThumbLX;
						int16_t StickY = Pad->sThumbLY;

						XOffset += StickX / 4096;
                        YOffset += StickY / 4096;

                        SoundOutput.ToneHz = 512 + (int)(256.0f*((float)StickY / 30000.0f));
                        SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
					}
					else
					{
						// NOTE(Matias): The controller is not available.
					}
				}

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackbuffer.Memory;
				Buffer.Width = GlobalBackbuffer.Width;
				Buffer.Height = GlobalBackbuffer.Height;
				Buffer.Pitch = GlobalBackbuffer.Pitch;
				GameUpdateAndRender(&Buffer, XOffset, YOffset);

				// NOTE(Matias): DirectSound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
					DWORD BytesToWrite;
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

					Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite);
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
