/* Public Domain, Simple, Minimalistic JPEG writer - http://jonolick.com
 *
 * Quick Notes:
 * 	Based on a javascript jpeg writer
 * 	JPEG baseline (no JPEG progressive)
 * 	Supports 1, 3 or 4 component input. (luminance, RGB or RGBX)
 *
 * Latest revisions:
 *  1.60 (2019-27-11) Added support for subsampling U,V so that it encodes smaller files. Enabled when quality <= 90.
 *	1.52 (2012-22-11) Added support for specifying Luminance, RGB, or RGBA via comp(onents) argument (1, 3 and 4 respectively).
 *	1.51 (2012-19-11) Fixed some warnings
 *	1.50 (2012-18-11) MT safe. Simplified. Optimized. Reduced memory requirements. Zero allocations. No namespace polution. Approx 340 lines code.
 *	1.10 (2012-16-11) compile fixes, added docs,
 *		changed from .h to .cpp (simpler to bootstrap), etc
 * 	1.00 (2012-02-02) initial release
 *
 * Basic usage:
 *	char *foo = new char[128*128*4]; // 4 component. RGBX format, where X is unused
 *	jo_write_jpg("foo.jpg", foo, 128, 128, 4, 90); // comp can be 1, 3, or 4. Lum, RGB, or RGBX respectively.
 *
 * */

#define _CRT_SECURE_NO_WARNINGS


#ifndef JO_INCLUDE_JPEG_H
#define JO_INCLUDE_JPEG_H

 // To get a header file for this, either cut and paste the header,
 // or create jo_jpeg.h, #define JO_JPEG_HEADER_FILE_ONLY, and
 // then include jo_jpeg.c from it.

 // Returns false on failure
bool jo_write_jpg(const char *filename, const void *data, int width, int height,
                  int comp, int quality);

#endif // JO_INCLUDE_JPEG_H

#ifndef JO_JPEG_HEADER_FILE_ONLY

#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include "enc/tables.hpp"

FILE *fp2;

//std::ifstream indct;

static void jo_writeBits(std::ofstream& outFile, int &bitBuf, int &bitCnt,
                         const unsigned short *bs) {
    bitCnt += bs[1];
    bitBuf |= bs[0] << (24 - bitCnt);
    while (bitCnt >= 8) {
        unsigned char c = (bitBuf >> 16) & 255;
        outFile.put(c);
        if (c == 255) {
            outFile.put(0);
        }
        bitBuf <<= 8;
        bitCnt -= 8;
    }
}

static void jo_DCT(float &d0, float &d1, float &d2, float &d3, float &d4,
                   float &d5, float &d6, float &d7) {
    float tmp0 = d0 + d7;
    float tmp7 = d0 - d7;
    float tmp1 = d1 + d6;
    float tmp6 = d1 - d6;
    float tmp2 = d2 + d5;
    float tmp5 = d2 - d5;
    float tmp3 = d3 + d4;
    float tmp4 = d3 - d4;

    // Even part
    float tmp10 = tmp0 + tmp3;	// phase 2
    float tmp13 = tmp0 - tmp3;
    float tmp11 = tmp1 + tmp2;
    float tmp12 = tmp1 - tmp2;

    d0 = tmp10 + tmp11; 		// phase 3
    d4 = tmp10 - tmp11;

    float z1 = (tmp12 + tmp13) * 0.707106781f; // c4
    d2 = tmp13 + z1; 		// phase 5
    d6 = tmp13 - z1;

    // Odd part
    tmp10 = tmp4 + tmp5; 		// phase 2
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    // The rotator is modified from fig 4-8 to avoid extra negations.
    float z5 = (tmp10 - tmp12) * 0.382683433f; // c6
    float z2 = tmp10 * 0.541196100f + z5; // c2-c6
    float z4 = tmp12 * 1.306562965f + z5; // c2+c6
    float z3 = tmp11 * 0.707106781f; // c4

    float z11 = tmp7 + z3;		// phase 5
    float z13 = tmp7 - z3;

    d5 = z13 + z2;			// phase 6
    d3 = z13 - z2;
    d1 = z11 + z4;
    d7 = z11 - z4;
}

static void jo_calcBits(int val, unsigned short bits[2]) {
    int tmp1 = val < 0 ? -val : val;
    val = val < 0 ? val - 1 : val;
    bits[1] = 1;
    while (tmp1 >>= 1) {
        ++bits[1];
    }
    bits[0] = val & ((1 << bits[1]) - 1);
}

template <std::input_iterator IteratorT>
int jo_processDU(IteratorT& inDCT, std::ofstream& outJpeg,
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
            int n = *inDCT;
            ++inDCT;

            std::cout << " " << n;
            DU[tbl::s_jo_ZigZag[j]] = n;// (int)(v < 0 ? ceilf(v - 0.5f) : floorf(v + 0.5f));
        }
    }
    std::cout << "\n";
    int n;

    const auto writeJpegBits = [&outJpeg, &bitBuf, &bitCnt](auto bits) {
        jo_writeBits(outJpeg, bitBuf, bitCnt, bits);
    };

    // Encode DC
    int diff = DU[0] - DC;
    if (diff == 0) {
        writeJpegBits(HTDC[0]);
    }
    else {
        unsigned short bits[2];
        jo_calcBits(diff, bits);
        writeJpegBits(HTDC[bits[1]]);
        writeJpegBits(bits);
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
            for (int nrmarker = 1; nrmarker <= lng; ++nrmarker) {
                writeJpegBits(M16zeroes);
            }
            nrzeroes &= 15;
        }
        unsigned short bits[2];
        jo_calcBits(DU[i], bits);
        writeJpegBits(HTAC[(nrzeroes << 4) + bits[1]]);
        writeJpegBits(bits);
    }
    if (end0pos != 63) {
        writeJpegBits(EOB);
    }
    return DU[0];
}


struct QualityEstimation {
    int quality;
    bool subsample;
};

QualityEstimation estimQuality(int quality) {
    quality = quality ? quality : 90;
    bool subsample = quality <= 90 ? 1 : 0;
    quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
    quality = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return { quality, subsample };
}

std::array<unsigned char, 24> calcHead1(int width, int height, bool subsample) {
    unsigned char h0 = height >> 8;
    unsigned char h1 = height & 0xFF;
    unsigned char w0 = width >> 8;
    unsigned char w1 = width & 0xFF;
    return std::array<unsigned char, 24>{
        0xFF, 0xC0, 0, 0x11, 8, h0, h1, w0, w1, 3, 1,
        (unsigned char) (subsample ? 0x22 : 0x11), 0,
        2, 0x11, 1, 3, 0x11, 1, 0xFF, 0xC4, 0x01, 0xA2, 0
    };
}

bool jo_write_headers(std::ofstream& outBinary, int width, int height,
                      int comp, int quality) {
    if (!width || !height || comp > 4 || comp < 1 || comp == 2) {
        return false;
    }

    const auto [actualQuality, subsample] = estimQuality(quality);

    auto YTable = std::array<unsigned char, 64>();
    auto UVTable = std::array<unsigned char, 64>();

    for (int i = 0; i < 64; ++i) {
        int yti = (tbl::YQT[i] * actualQuality + 50) / 100;
        YTable[tbl::s_jo_ZigZag[i]] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
        int uvti = (tbl::UVQT[i] * actualQuality + 50) / 100;
        UVTable[tbl::s_jo_ZigZag[i]] = uvti < 1 ? 1 : uvti > 255 ? 255 : uvti;
    }

    float fdtbl_Y[64], fdtbl_UV[64];
    for (int row = 0, k = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col, ++k) {
            fdtbl_Y[k] = 1 / (YTable[tbl::s_jo_ZigZag[k]] * tbl::aasf[row] * tbl::aasf[col]);
            fdtbl_UV[k] = 1 / (UVTable[tbl::s_jo_ZigZag[k]] * tbl::aasf[row] * tbl::aasf[col]);
        }
    }

    const auto writeArr =
        [&outBinary](const auto& arr, std::size_t startOffset = 0) {
            outBinary.write(reinterpret_cast<const char*>(arr.data() + startOffset),
                            arr.size() - startOffset);
        };

    // Write Headers
    writeArr(tbl::head0);
    writeArr(YTable);
    outBinary.put(1);
    writeArr(UVTable);
    writeArr(calcHead1(width, height, subsample));
    writeArr(tbl::std_dc_luminance_nrcodes, 1);
    writeArr(tbl::std_dc_luminance_values);
    outBinary.put(0x10); // HTYACinfo
    writeArr(tbl::std_ac_luminance_nrcodes, 1);
    writeArr(tbl::std_ac_luminance_values);
    outBinary.put(1); // HTUDCinfo
    writeArr(tbl::std_dc_chrominance_nrcodes, 1);
    writeArr(tbl::std_dc_chrominance_values);
    outBinary.put(0x11); // HTUACinfo
    writeArr(tbl::std_ac_chrominance_nrcodes, 1);
    writeArr(tbl::std_ac_chrominance_values);
    writeArr(tbl::head2);
    return true;
}

template <std::input_iterator IteratorT>
bool jo_write_jpg(IteratorT& inDCT, std::ofstream& outJpeg,
                  int width, int height, int comp, int quality) {
    if (!width || !height || comp > 4 || comp < 1 || comp == 2) {
        return false;
    }

    if (!jo_write_headers(outJpeg, width, height, comp, quality)) {
        return false;
    }

    const auto [actualQuality, subsample] = estimQuality(quality);

    unsigned char YTable[64], UVTable[64];
    for (int i = 0; i < 64; ++i) {
        int yti = (tbl::YQT[i] * actualQuality + 50) / 100;
        YTable[tbl::s_jo_ZigZag[i]] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
        int uvti = (tbl::UVQT[i] * actualQuality + 50) / 100;
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
    int DCY = 0, DCU = 0, DCV = 0;
    int bitBuf = 0, bitCnt = 0;
    const auto processDU = [&inDCT, &outJpeg, &bitBuf, &bitCnt](auto&&... tailArgs) {
        return jo_processDU(inDCT, outJpeg, bitBuf, bitCnt, tailArgs...);
    };

    if (subsample) {
        for (int y = 0; y < height; y += 16) {
            for (int x = 0; x < width; x += 16) {
                float Y[256], U[256], V[256];
                for (int row = y, pos = 0; row < y + 16; ++row) {
                    for (int col = x; col < x + 16; ++col, ++pos) {
                        int prow = row >= height ? height - 1 : row;
                        int pcol = col >= width ? width - 1 : col;
                        int p = prow * width*comp + pcol * comp;
                    }
                }
                DCY = processDU(Y + 0, 16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCY = processDU(Y + 8, 16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCY = processDU(Y + 128, 16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCY = processDU(Y + 136, 16, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                // subsample U,V
                float subU[64], subV[64];
                for (int yy = 0, pos = 0; yy < 8; ++yy) {
                    for (int xx = 0; xx < 8; ++xx, ++pos) {
                        int j = yy * 32 + xx * 2;
                        subU[pos] = (U[j + 0] + U[j + 1] + U[j + 16] + U[j + 17]) * 0.25f;
                        subV[pos] = (V[j + 0] + V[j + 1] + V[j + 16] + V[j + 17]) * 0.25f;
                    }
                }
                DCU = processDU(subU, 8, fdtbl_UV, DCU, tbl::UVDC_HT, tbl::UVAC_HT);
                DCV = processDU(subV, 8, fdtbl_UV, DCV, tbl::UVDC_HT, tbl::UVAC_HT);
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
                    }
                }
                DCY = processDU(Y, 8, fdtbl_Y, DCY, tbl::YDC_HT, tbl::YAC_HT);
                DCU = processDU(U, 8, fdtbl_UV, DCU, tbl::UVDC_HT, tbl::UVAC_HT);
                DCV = processDU(V, 8, fdtbl_UV, DCV, tbl::UVDC_HT, tbl::UVAC_HT);
            }
        }
    }

    // Do the bit alignment of the EOI marker
    static const unsigned short fillBits[] = { 0x7F, 7 };
    jo_writeBits(outJpeg, bitBuf, bitCnt, fillBits);
    outJpeg.put(0xFF);
    outJpeg.put(0xD9);
    return true;
}

int main(int argc, char* argv[])
{
    std::ifstream inDCTFile;
    inDCTFile.open(argv[1]);

    if (!inDCTFile.is_open()) {
        return 1;
    }

    std::istream_iterator<int> inDCTIter(inDCTFile);

    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    int comp = atoi(argv[4]);
    int quality = atoi(argv[5]);

    std::ofstream outJpeg;
    outJpeg.open(argv[6], std::ios::out | std::ios::binary);

    if (!outJpeg.is_open()) {
        return 1;
    }

    jo_write_jpg(inDCTIter, outJpeg, width, height, comp, quality);

    std::ofstream outHeaders;
    outHeaders.open("headers.bin", std::ios::out | std::ios::binary);

    if (!outHeaders.is_open()) {
        return 1;
    }

    jo_write_headers(outHeaders, width, height, comp, quality);
}



#endif
