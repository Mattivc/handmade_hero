#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global static

struct win32_offscreen_buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension {
	int Width;
	int Heigth;
};

global bool GlobalRunning;
global win32_offscreen_buffer GlobalBackbuffer;

internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
	win32_window_dimension Dimension;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Dimension.Heigth = ClientRect.bottom - ClientRect.top;
	Dimension.Width = ClientRect.right - ClientRect.left;

	return Dimension;
}

internal void RenderWierdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset) {

	// TODO(Matias): See what the optimizer does
	
	uint8_t *Row = (uint8_t *)Buffer.Memory;
	for (int Y = 0; Y < Buffer.Height; ++Y) {

		uint32_t *Pixel = (uint32_t *)Row;

		for (int X = 0; X < Buffer.Width; ++X) {

			uint8_t Blue = (X + XOffset);
			uint8_t Green = (Y + YOffset);

			*Pixel++ = (Green << 8 | Blue);
		}
		Row += Buffer.Pitch;
	}
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Heigth) {

	// TODO(Matias): Bulletproof this, free after, then free first if that fail
	// TODO(Matias): Free our DIBSection

	if(Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Heigth;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	
	
	int BitmapMemorySize = (Buffer->BytesPerPixel)*Width*Heigth;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	// TODO(Matias): Clear to black

	Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferToWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Heigth) {

	// TODO(Matias): Aspect ratio correction
	StretchDIBits(DeviceContext,
		          0, 0, WindowWidth, WindowHeight,
		          0, 0, Buffer.Width, Buffer.Height,
		          Buffer.Memory,
		          &Buffer.Info,
		          DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK Win32MainWindowCallback(
  HWND   Window,
  UINT   Message,
  WPARAM wParam,
  LPARAM lParam) {

	LRESULT Result = 0;

	switch(Message) {

		case WM_SIZE: {
		} break;

		case WM_CLOSE: {
			// TODO(Matias): Handle this with a message to the user
			GlobalRunning = false;
		} break;

		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY: {
			// TODO(Matias): Handle this as an error - recreate window
			GlobalRunning = false;
		} break;

		case WM_PAINT: {
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);

			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Heigth = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferToWindow(DeviceContext, Dimension.Width, Dimension.Heigth, GlobalBackbuffer, X, Y, Width, Heigth);

			EndPaint(Window, &Paint);
		} break;

		default: {
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
	}

	return Result;
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       CmdShow) {

	WNDCLASS WindowClass = {};

	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	// WindowClass.hIcon;

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.lpszClassName = "HandmadeHeroWindowClass";
	if(RegisterClass(&WindowClass)) {

		HWND Window = CreateWindowEx(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);

		if(Window) {
			int XOffset = 0;
			int YOffset = 0;

			GlobalRunning = true;
			while(GlobalRunning) {
				MSG Message;

				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
					if(Message.message == WM_QUIT) {
						GlobalRunning = false;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderWierdGradient(GlobalBackbuffer, XOffset, YOffset);
				HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferToWindow(DeviceContext, Dimension.Width, Dimension.Heigth, GlobalBackbuffer,
					                       0, 0, Dimension.Heigth, Dimension.Width);
				ReleaseDC(Window, DeviceContext);

				++XOffset;
				++YOffset;
			}
		} else {
			// TODO(Matias): Logging
		}

	} else {
		// TODO(Matias): Logging
	}

	return 0;
}
