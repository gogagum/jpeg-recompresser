#include <fstream>
#include <cstdio>
#include <iostream>
#include "tables.hpp"

#ifndef JO_INCLUDE_JPEG_HPP
#define JO_INCLUDE_JPEG_HPP

//#include <stdio.h>

 // To get a header file for this, either cut and paste the header,
 // or create jo_jpeg.h, #define JO_JPEG_HEADER_FILE_ONLY, and
 // then include jo_jpeg.c from it.


 // Returns false on failure
template <class T>
bool jo_write_jpg(T iterator, const char *filename, const void *data, int width, int height, int comp, int quality);

bool
jo_write_headers(std::ofstream& outBinHeaders, const void *data,
                 int width, int height, int comp, int quality);

template <class T>
int jo_processDU(T& iterator, std::ofstream& outJpeg, int &bitBuf, int &bitCnt, float *CDU, int du_stride,
                 float *fdtbl, int DC,
                 const unsigned short HTDC[256][2],
                 const unsigned short HTAC[256][2]);

void jo_writeBits(std::ofstream& outJpeg, int &bitBuf, int &bitCnt, const unsigned short *bs);


static void
jo_DCT(float &d0, float &d1, float &d2, float &d3, float &d4, float &d5, float &d6, float &d7);

static void jo_calcBits(int val, unsigned short bits[2]);

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
template <class T>
bool jo_write_jpg(T inDCTIterator, std::ofstream& outJpeg, const void *data, int width, int height, int comp, int quality) {
    if (!data || !width || !height || comp > 4 || comp < 1 || comp == 2) {
        return false;
    }

    if (!jo_write_headers(outJpeg, data, width, height, comp, quality)) {
        return false;
    }

    quality = quality ? quality : 90;
    const int subsample = quality <= 90 ? 1 : 0;
    quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
    quality = quality < 50 ? 5000 / quality : 200 - quality * 2;


    unsigned char YTable[64], UVTable[64];
    for (int i = 0; i < 64; ++i) {
        int yti = (tbl::YQT[i] * quality + 50) / 100;
        YTable[tbl::s_jo_ZigZag[i]] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
        int uvti = (tbl::UVQT[i] * quality + 50) / 100;
        UVTable[tbl::s_jo_ZigZag[i]] = uvti < 1 ? 1 : uvti > 255 ? 255 : uvti;
    }

    float fdtbl_Y[64], fdtbl_UV[64];
    for (int row = 0, k = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col, ++k) {
            fdtbl_Y[k] = 1 / (YTable[tbl::s_jo_ZigZag[k]] * tbl::aasf[row] * tbl::aasf[col]);
            fdtbl_UV[k] = 1 / (UVTable[tbl::s_jo_ZigZag[k]] * tbl::aasf[row] * tbl::aasf[col]);
        }
    }

    // Encode 8x8 macroblocks
    int ofsG = comp > 1 ? 1 : 0, ofsB = comp > 1 ? 2 : 0;
    const unsigned char *dataR = (const unsigned char *)data;
    const unsigned char *dataG = dataR + ofsG;
    const unsigned char *dataB = dataR + ofsB;
    int DCY = 0, DCU = 0, DCV = 0;
    int bitBuf = 0, bitCnt = 0;
    if (subsample) {
        for (int y = 0; y < height; y += 16) {
            for (int x = 0; x < width; x += 16) {
                float Y[256], U[256], V[256];
                for (int row = y, pos = 0; row < y + 16; ++row) {
                    for (int col = x; col < x + 16; ++col, ++pos) {
                        int prow = row >= height ? height - 1 : row;
                        int pcol = col >= width ? width - 1 : col;
                        int p = prow * width*comp + pcol * comp;
                        float r = dataR[p], g = dataG[p], b = dataB[p];
                        Y[pos] = +0.29900f*r + 0.58700f*g + 0.11400f*b - 128;
                        U[pos] = -0.16874f*r - 0.33126f*g + 0.50000f*b;
                        V[pos] = +0.50000f*r - 0.41869f*g - 0.08131f*b;
                    }
                }
                DCY = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, Y + 0,   16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCY = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, Y + 8,   16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCY = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, Y + 128, 16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCY = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, Y + 136, 16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                // subsample U,V
                float subU[64], subV[64];
                for (int yy = 0, pos = 0; yy < 8; ++yy) {
                    for (int xx = 0; xx < 8; ++xx, ++pos) {
                        int j = yy * 32 + xx * 2;
                        subU[pos] = (U[j + 0] + U[j + 1] + U[j + 16] + U[j + 17]) * 0.25f;
                        subV[pos] = (V[j + 0] + V[j + 1] + V[j + 16] + V[j + 17]) * 0.25f;
                    }
                }
                DCU = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, subU, 8, fdtbl_UV, DCU, tbl::UVDC_HT, tbl::UVAC_HT);
                DCV = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, subV, 8, fdtbl_UV, DCV, tbl::UVDC_HT, tbl::UVAC_HT);
            }
        }
    }
    else {
        for (int y = 0; y < height; y += 8) {
            for (int x = 0; x < width; x += 8) {
                float Y[64], U[64], V[64];
                for (int row = y, pos = 0; row < y + 8; ++row) {
                    for (int col = x; col < x + 8; ++col, ++pos) {
                        int prow = row >= height ? height - 1 : row;
                        int pcol = col >= width ? width - 1 : col;
                        int p = prow * width*comp + pcol * comp;
                        float r = dataR[p], g = dataG[p], b = dataB[p];
                        Y[pos] = +0.29900f*r + 0.58700f*g + 0.11400f*b - 128;
                        U[pos] = -0.16874f*r - 0.33126f*g + 0.50000f*b;
                        V[pos] = +0.50000f*r - 0.41869f*g - 0.08131f*b;
                    }
                }
                DCY = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, Y, 8, fdtbl_Y, DCY,  tbl::YDC_HT,  tbl::YAC_HT);
                DCU = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, U, 8, fdtbl_UV, DCU, tbl::UVDC_HT, tbl::UVAC_HT);
                DCV = jo_processDU(inDCTIterator, outJpeg, bitBuf, bitCnt, V, 8, fdtbl_UV, DCV, tbl::UVDC_HT, tbl::UVAC_HT);
            }
        }
    }

    // Do the bit alignment of the EOI marker
    static const unsigned short fillBits[] = { 0x7F, 7 };
    jo_writeBits(outJpeg, bitBuf, bitCnt, fillBits);
    outJpeg.put(0xFF);
    //putc(0xFF, fp);
    outJpeg.put(0xD9);
    //putc(0xD9, fp);
    //fclose(fp);
    return true;
}

template <class T>
int jo_processDU(
            T& indctIterator,
            std::ofstream& outJpeg,
            int &bitBuf, int &bitCnt, float *CDU, int du_stride,
            float *fdtbl, int DC,
            const unsigned short HTDC[256][2],
            const unsigned short HTAC[256][2]) {
    const unsigned short EOB[2] = { HTAC[0x00][0], HTAC[0x00][1] };
    const unsigned short M16zeroes[2] = { HTAC[0xF0][0], HTAC[0xF0][1] };

    // DCT rows
    for (int i = 0; i < du_stride * 8; i += du_stride) {
        jo_DCT(CDU[i],     CDU[i + 1], CDU[i + 2], CDU[i + 3],
               CDU[i + 4], CDU[i + 5], CDU[i + 6], CDU[i + 7]);
    }
    // DCT columns
    for (int i = 0; i < 8; ++i) {
        jo_DCT(CDU[i],                 CDU[i + du_stride],
               CDU[i + du_stride * 2], CDU[i + du_stride * 3],
               CDU[i + du_stride * 4], CDU[i + du_stride * 5],
               CDU[i + du_stride * 6], CDU[i + du_stride * 7]);
    }

    // Quantize/descale/zigzag the coefficients
    int DU[64];
    for (int y = 0, j = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x, ++j) {
            //int i = y * du_stride + x;
            //float v = CDU[i] * fdtbl[j];
            int n = *indctIterator;
            ++indctIterator;
            //indct >> n;

            std::cout << " " << n;
            DU[tbl::s_jo_ZigZag[j]] =n;// (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
        }
        //cout << "\n";
    }
    std::cout << std::endl;
    int n;
    for (int j=0; j<64; j++){
      //  indct >> n;
        //cout << n << " ";
      //  DU[j] = n;
    }
    for (int j=0; j <64; j++){
        //cout << DU[j] << " ";
    }
    //cout << "\n";

    // Encode DC
    int diff = DU[0] - DC;
    if (diff == 0) {
        jo_writeBits(outJpeg, bitBuf, bitCnt, HTDC[0]);
    }
    else {
        unsigned short bits[2];
        jo_calcBits(diff, bits);
        jo_writeBits(outJpeg, bitBuf, bitCnt, HTDC[bits[1]]);
        jo_writeBits(outJpeg, bitBuf, bitCnt, bits);
    }
    // Encode ACs
    int end0pos = 63;
    for (; (end0pos > 0) && (DU[end0pos] == 0); --end0pos) {
    }
    // end0pos = first element in reverse order !=0
    if (end0pos == 0) {
        jo_writeBits(outJpeg, bitBuf, bitCnt, EOB);
        return DU[0];
    }
    for (int i = 1; i <= end0pos; ++i) {
        int startpos = i;
        for (; DU[i] == 0 && i <= end0pos; ++i) {
        }
        int nrzeroes = i - startpos;
        if (nrzeroes >= 16) {
            int lng = nrzeroes >> 4;
            for (int nrmarker = 1; nrmarker <= lng; ++nrmarker)
                jo_writeBits(outJpeg, bitBuf, bitCnt, M16zeroes);
            nrzeroes &= 15;
        }
        unsigned short bits[2];
        jo_calcBits(DU[i], bits);
        jo_writeBits(outJpeg, bitBuf, bitCnt, HTAC[(nrzeroes << 4) + bits[1]]);
        jo_writeBits(outJpeg, bitBuf, bitCnt, bits);
    }
    if (end0pos != 63) {
        jo_writeBits(outJpeg, bitBuf, bitCnt, EOB);
    }
    return DU[0];
}




#endif // JO_INCLUDE_JPEG_HPP
