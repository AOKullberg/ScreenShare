#ifndef __BMPFUNCS__
#define __BMPFUNCS__

// Functions to read and write to greyscale image files
unsigned char* readGreyscaleImage(const char *filename, int* widthOut, int* heightOut);
void storeGreyscaleImage(unsigned char* imageOut, const char *filename, int rows, int cols, const char* refFilename);

// Functions to read and write to RGB colour image files, returns colour in RGBA format
unsigned char* readRGBImage(const char *filename, int* widthOut, int* heightOut);
void storeRGBImage(unsigned char* imageOut, const char *filename, int rows, int cols, const char* refFilename);

#endif
