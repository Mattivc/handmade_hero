#include <windows.h>

#define internal static
#define local_persist static
#define global static

global bool Running;

global BITMAPINFO BitmapInfo;
global void *BitmapMemory;\
global HBITMAP BitmapHandle;
global HDC BitmapDeviceContext;

internal void Win32ResizeDIBSection(int Width, int Heigth) {

	// TODO(Matias): Bulletproof this, free after, then free first if that fail
	// TODO(Matias): Free our DIBSection
	
	if(BitmapHandle) {
		DeleteObject(BitmapHandle);
	}

	if(!BitmapDeviceContext){
		// TODO(Matias): Should we recreate these under certian circumstances
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Heigth;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	BitmapHandle = CreateDIBSection(
		BitmapDeviceContext, &BitmapInfo,
		DIB_RGB_COLORS,
		&BitmapMemory,
		0, 0);
}

internal void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Heigth) {
	StretchDIBits(
		DeviceContext,
		X, Y, Width, Heigth,
		X, Y, Width, Heigth,
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

			Win32UpdateWindow(DeviceContext, X, Y, Width, Heigth);

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

		HWND WindowHandle = CreateWindowEx(
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

		if(WindowHandle) {
			Running = true;
			while(Running) {
				MSG Message;
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);	

				if (MessageResult > 0) {
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				} else { break; }
			}

		} else {
			// TODO(Matias): Logging
		}

	} else {
		// TODO(Matias): Logging
	}

	return(0);
}
