#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global static


global bool Running;
global BITMAPINFO BitmapInfo;
global void *BitmapMemory;
global int BitmapWidth;
global int BitmapHeight;
global int BytesPerPixel = 4;

internal void RenderWierdGradient(int XOffset, int YOffset) {

	int Width = BitmapWidth;
	int Heigth = BitmapHeight;

	int Pitch = Width*BytesPerPixel;
	uint8_t *Row = (uint8_t *)BitmapMemory;
	for (int Y = 0; Y < BitmapHeight; ++Y) {

		uint32_t *Pixel = (uint32_t *)Row;

		for (int X = 0; X < BitmapWidth; ++X) {

			uint8_t Blue = (X + XOffset);
			uint8_t Green = (Y + YOffset);

			*Pixel++ = (Green << 8 | Blue);
		}
		Row += Pitch;
	}
}

internal void Win32ResizeDIBSection(int Width, int Heigth) {

	// TODO(Matias): Bulletproof this, free after, then free first if that fail
	// TODO(Matias): Free our DIBSection

	if(BitmapMemory) {
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Heigth;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	
	int BitmapMemorySize = BytesPerPixel*Width*Heigth;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	// TODO(Matias): Clear to black
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int X, int Y, int Width, int Heigth) {

	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top;

	StretchDIBits(
		DeviceContext,
		/*
		X, Y, Width, Heigth,
		X, Y, Width, Heigth,
		*/
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory,
		&BitmapInfo,
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
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Heigth = ClientRect.bottom - ClientRect.top;
			int Width = ClientRect.right - ClientRect.left;
			Win32ResizeDIBSection(Width, Heigth);
		} break;

		case WM_CLOSE: {
			// TODO(Matias): Handle this with a message to the user
			Running = false;
		} break;

		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY: {
			// TODO(Matias): Handle this as an error - recreate window
			Running = false;
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

			Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Heigth);

			EndPaint(Window, &Paint);
		} break;

		default: {
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
	}

	return(Result);
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       CmdShow) {

	WNDCLASS WindowClass = {};

	// TODO(Matias): Check if CS_OWNDC|CS_HREDRAW|CS_VREDRAW matter
	//WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	// WindowClass.hIcon;

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

			Running = true;
			while(Running) {
				MSG Message;

				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
					if(Message.message == WM_QUIT) {
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderWierdGradient(XOffset, YOffset);
				HDC DeviceContext = GetDC(Window);

				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				int Heigth = ClientRect.bottom - ClientRect.top;
				int Width = ClientRect.right - ClientRect.left;

				Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, Heigth, Width);
				ReleaseDC(Window, DeviceContext);

				++XOffset;
			}
		} else {
			// TODO(Matias): Logging
		}

	} else {
		// TODO(Matias): Logging
	}

	return(0);
}
