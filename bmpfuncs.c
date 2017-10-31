#include <stdio.h>
#include <stdlib.h>

#include "bmpfuncs.h"

/*
 * Read 8-bit greyscale bmp image into a byte array. Also output the width and height
 */
unsigned char* readGreyscaleImage(const char *filename, int* widthOut, int* heightOut) {

   FILE *fp;
   unsigned char* imageData;

   int height, width;
   unsigned char tmp;
   int offset;
   int i, j;
   int mod;

   printf("Reading input image from %s\n", filename);
   fp = fopen(filename, "rb");
   if(fp == NULL) {
       perror(filename);
       exit(-1);
   }

   fseek(fp, 10, SEEK_SET);
   fread(&offset, 4, 1, fp);

   fseek(fp, 18, SEEK_SET);
   fread(&width, 4, 1, fp);
   fread(&height, 4, 1, fp);

   printf("width = %d\n", width);
   printf("height = %d\n", height);

   *widthOut = width;
   *heightOut = height;    

   imageData = (unsigned char*)malloc(width*height);
   if(imageData == NULL) {
       perror("malloc");
       exit(-1);
   }

   fseek(fp, offset, SEEK_SET);
   fflush(NULL);

   mod = width % 4;
   if(mod != 0) {
       mod = 4 - mod;
   }

   // NOTE bitmaps are stored in upside-down raster order.  So we begin
   // reading from the bottom left pixel, then going from left-to-right, 
   // read from the bottom to the top of the image.  For image analysis, 
   // we want the image to be right-side up, so we'll modify it here.

   for(i = height-1; i >= 0; i--) {
      fread(&imageData[i*width], sizeof(char), width, fp);

	  // For the bmp format, each row has to be a multiple of 4, 
      // so I need to read in the junk data and throw it away
      for(j = 0; j < mod; j++) {
         fread(&tmp, sizeof(char), 1, fp);
      }
   }

   fclose(fp);

   return imageData;
}

/*
 * Store contents into an 8-bit greyscale bmp image
 */

void storeGreyscaleImage(unsigned char* imageOut, const char *filename, int rows, int cols, 
                const char* refFilename) {

   FILE *ifp, *ofp;
   unsigned char tmp;
   int offset;
   unsigned char *buffer;
   int i, j;

   int bytes;

   int height, width;
   int mod;

   ifp = fopen(refFilename, "rb");
   if(ifp == NULL) {
      perror(filename);
      exit(-1);
   }

   fseek(ifp, 10, SEEK_SET);
   fread(&offset, 4, 1, ifp);

   fseek(ifp, 18, SEEK_SET);
   fread(&width, 4, 1, ifp);
   fread(&height, 4, 1, ifp);

   fseek(ifp, 0, SEEK_SET);

   buffer = (unsigned char *)malloc(offset);
   if(buffer == NULL) {
      perror("malloc");
      exit(-1);
   }

   fread(buffer, 1, offset, ifp);

   printf("Writing output image to %s\n", filename);
   ofp = fopen(filename, "wb");
   if(ofp == NULL) {
      perror("opening output file");
      exit(-1);
   }
   bytes = fwrite(buffer, 1, offset, ofp);
   if(bytes != offset) {
      printf("error writing header!\n");
      exit(-1);
   }

   // NOTE bmp formats store data in reverse raster order (see comment in
   // readImage function), so we need to flip it upside down here.  
   mod = width % 4;
   if(mod != 0) {
      mod = 4 - mod;
   }
   for(i = height-1; i >= 0; i--) {
      fwrite(&imageOut[i*cols], sizeof(char), width, ofp);

	  // In bmp format, rows must be a multiple of 4-bytes.  
      // So if we're not at a multiple of 4, add junk padding.
      for(j = 0; j < mod; j++) {
         fwrite(&tmp, sizeof(char), 1, ofp);
      }
   } 

   fclose(ofp);
   fclose(ifp);

   free(buffer);
}


/*
 * Read from a 24-bit RGB bmp image, convert to RGBA format and store in a byte array. Also output the width and height
 */
unsigned char* readRGBImage(const char *filename, int* widthOut, int* heightOut) {

   FILE *fp;
   unsigned char* imageData;
   float* floatImage = NULL;

   int height, width;
   unsigned char tmp[3];
   int offset;
   int i, j;
   int mod;
   int rowsize;

   printf("Reading input image from %s\n", filename);
   fp = fopen(filename, "rb");
   if(fp == NULL) {
       perror(filename);
       exit(-1);
   }

   fseek(fp, 10, SEEK_SET);
   fread(&offset, 4, 1, fp);

   fseek(fp, 18, SEEK_SET);
   fread(&width, 4, 1, fp);
   fread(&height, 4, 1, fp);

   printf("width = %d\n", width);
   printf("height = %d\n", height);

   *widthOut = width;
   *heightOut = height;    

   imageData = (unsigned char*)malloc(width*height*4);
   if(imageData == NULL) {
       perror("malloc");
       exit(-1);
   }

   fseek(fp, offset, SEEK_SET);
   fflush(NULL);

   mod = width % 4;
   if(mod != 0) {
       mod = 4 - mod;
   }

   // NOTE bitmaps are stored in upside-down raster order.  So we begin
   // reading from the bottom left pixel, then going from left-to-right, 
   // read from the bottom to the top of the image.  For image analysis, 
   // we want the image to be right-side up, so we'll modify it here.

   rowsize = width*4;

   for(i = height-1; i >= 0; i--) {
      for(j = 0; j < rowsize; j+=4) {
         fread(tmp, sizeof(char), 3, fp);
         imageData[i*rowsize + j] = tmp[0];
         imageData[i*rowsize + j+1] = tmp[1];
         imageData[i*rowsize + j+2] = tmp[2];
         imageData[i*rowsize + j+3] = 255;
      }
      // For the bmp format, each row has to be a multiple of 4, 
      // so I need to read in the junk data and throw it away
      for(j = 0; j < mod; j++) {
         fread(tmp, sizeof(char), 3, fp);
      }
   }

   fclose(fp);

   return imageData;
}

/*
 * Accepts an image array in RGBA format and stores contents in a 24-bit RGB bmp image
 */
void storeRGBImage(unsigned char* imageOut, const char *filename, int rows, int cols, 
                const char* refFilename) {

   FILE *ifp, *ofp;
   unsigned char tmp[3];
   int offset;
   unsigned char *buffer;
   int i, j;

   int bytes;

   int height, width;
   int mod;

   ifp = fopen(refFilename, "rb");
   if(ifp == NULL) {
      perror(filename);
      exit(-1);
   }

   fseek(ifp, 10, SEEK_SET);
   fread(&offset, 4, 1, ifp);

   fseek(ifp, 18, SEEK_SET);
   fread(&width, 4, 1, ifp);
   fread(&height, 4, 1, ifp);

   fseek(ifp, 0, SEEK_SET);

   buffer = (unsigned char *)malloc(offset);
   if(buffer == NULL) {
      perror("malloc");
      exit(-1);
   }

   fread(buffer, 1, offset, ifp);

   printf("Writing output image to %s\n", filename);
   ofp = fopen(filename, "wb");
   if(ofp == NULL) {
      perror("opening output file");
      exit(-1);
   }
   bytes = fwrite(buffer, 1, offset, ofp);
   if(bytes != offset) {
      printf("error writing header!\n");
      exit(-1);
   }

   // NOTE bmp formats store data in reverse raster order (see comment in
   // readImage function), so we need to flip it upside down here.  
   mod = width % 4;
   if(mod != 0) {
      mod = 4 - mod;
   }


   for(i = height-1; i >= 0; i--) {
      for(j = 0; j < width*4; j+=4) {
         tmp[0] = (unsigned char)imageOut[i*cols*4+j];
         tmp[1] = (unsigned char)imageOut[i*cols*4+j+1];
         tmp[2] = (unsigned char)imageOut[i*cols*4+j+2];
         fwrite(tmp, sizeof(char), 3, ofp);
      }

	  // In bmp format, rows must be a multiple of 4-bytes.  
      // So if we're not at a multiple of 4, add junk padding.
      for(j = 0; j < mod; j++) {
         fwrite(tmp, sizeof(char), 3, ofp);
      }
   } 

   fclose(ofp);
   fclose(ifp);

   free(buffer);
}
