#include <stdio.h>

#ifndef JO_INCLUDE_JPEG_HPP
#define JO_INCLUDE_JPEG_HPP

//#include <stdio.h>

 // To get a header file for this, either cut and paste the header,
 // or create jo_jpeg.h, #define JO_JPEG_HEADER_FILE_ONLY, and
 // then include jo_jpeg.c from it.


 // Returns false on failure
bool jo_write_jpg(const char *filename, const void *data, int width, int height, int comp, int quality);

int jo_processDU(FILE *fp, int &bitBuf, int &bitCnt, float *CDU, int du_stride,
                 float *fdtbl, int DC,
                 const unsigned short HTDC[256][2],
                 const unsigned short HTAC[256][2]);

void jo_writeBits(FILE *fp, int &bitBuf, int &bitCnt, const unsigned short *bs);

#endif // JO_INCLUDE_JPEG_HPP
