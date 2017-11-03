#include "screenshot.h"

extern "C" {
#include "bmpfuncs.h"
}

using namespace std;
using namespace Gdiplus;

/* Global variables */
ULONG_PTR gdiplusToken;
GdiplusStartupInput gdiplusStartupInput;
HWND hMyWnd;
int w, h;
HDC dc, hdcCapture;
int nBPP;
char* cur_screen = NULL;
char* buf_screen = NULL;
HBITMAP current_image;
HBITMAP buffer_image;

void initGDI() {
	RECT r;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	hMyWnd = GetDesktopWindow();
	GetWindowRect(hMyWnd, &r);
	dc = GetWindowDC(hMyWnd);   // GetDC(hMyWnd) ;
	w = r.right - r.left;
	h = r.bottom - r.top;
	w = 1920;
	h = 1080;
	nBPP = GetDeviceCaps(dc, BITSPIXEL);
	hdcCapture = CreateCompatibleDC(dc);
}

void shutdownGDI() {
	DeleteDC(hdcCapture);
	DeleteDC(dc);
	GdiplusShutdown(gdiplusToken);
	DeleteObject(buffer_image);
	DeleteObject(current_image);
}

unsigned char* GetScreenshot()
{
	int nCapture;
	LPBYTE lpCapture;
	// create the buffer for the screenshot
	BITMAPINFO bmiCapture = { sizeof(BITMAPINFOHEADER), w, -h, 1, nBPP, BI_RGB, 0, 0, 0, 0, 0, };

	// Take the screenshot
	buffer_image = CreateDIBSection(dc, &bmiCapture, DIB_PAL_COLORS, (LPVOID *)&lpCapture, NULL, 0);

	// Failure 
	if (!buffer_image) {
		DeleteDC(hdcCapture);
		DeleteDC(dc);
		GdiplusShutdown(gdiplusToken);
		printf("failed to take the screenshot. err: %d\n", GetLastError());
		return 0;
	}

	// copy the screenshot buffer
	nCapture = SaveDC(hdcCapture);
	SelectObject(hdcCapture, buffer_image);
	BitBlt(hdcCapture, 0, 0, w, h, dc, 0, 0, SRCCOPY);
	RestoreDC(hdcCapture, nCapture);
	return (unsigned char*)lpCapture;
}
/*
int main() {
	initGDI();
	// Tar bilder i ca 30 fps 
	GetScreenshot();
	storeRGBImage((unsigned char*)buf_screen, "output.bmp", h, w, "cmp.bmp");
	cout << "Done";
	getchar();

	shutdownGDI();

	return 0;
}*/