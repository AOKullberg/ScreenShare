#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

void initGDI();
void shutdownGDI();
unsigned char* GetScreenshot();