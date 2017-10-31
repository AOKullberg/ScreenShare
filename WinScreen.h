#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")


struct Dims {
	int width;
	int height;
};

class Screen {
public:
	Screen() { initGDI(); }
	~Screen() { shutdownGDI(); }

	/* Returns a screenshot of the windows desktop. */
	unsigned char* getScreenshot();
	Dims getDims();


private:
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	HWND hMyWnd;
	int w, h;
	HDC dc, hdcCapture;
	int nBPP;
	char* cur_screen = NULL;
	char* buf_screen = NULL;
	HBITMAP current_image;
	HBITMAP buffer_image;

	void initGDI();
	void shutdownGDI();
};