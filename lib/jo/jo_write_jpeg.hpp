#ifndef JO_WRITE_JPEG_HPP
#define JO_WRITE_JPEG_HPP

#include <array>
#include <iterator>
#include <fstream>

#include "tables.hpp"

void jo_writeBits(std::ofstream& outFile, int &bitBuf, int &bitCnt,
                         const unsigned short *bs);

struct QualityEstimation {
    int quality;
    bool subsample;
};

QualityEstimation estimQuality(int quality);

void jo_DCT(float &d0, float &d1, float &d2, float &d3, float &d4,
            float &d5, float &d6, float &d7);

void jo_calcBits(int val, unsigned short bits[2]);

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

            DU[tbl::s_jo_ZigZag[j]] = n;
        }
    }
    int n;

    const auto writeJpegBits = [&outJpeg, &bitBuf, &bitCnt](auto bits) {
        jo_writeBits(outJpeg, bitBuf, bitCnt, bits);
    };

    // Encode DC
    int diff = DU[0] - DC;
    if (diff == 0) {
        writeJpegBits(HTDC[0]);
    } else {
        unsigned short bits[2];
        jo_calcBits(diff, bits);
        writeJpegBits(HTDC[bits[1]]);
        writeJpegBits(bits);
    }
    // Encode ACs
    int end0pos = 63;
    for (; (end0pos > 0) && (DU[end0pos] == 0); --end0pos) {}
    // end0pos = first element in reverse order !=0
    if (end0pos == 0) {
        jo_writeBits(outJpeg, bitBuf, bitCnt, EOB);
        return DU[0];
    }
    for (int i = 1; i <= end0pos; ++i) {
        int startpos = i;
        for (; DU[i] == 0 && i <= end0pos; ++i) {}
        int nrzeroes = i - startpos;
        if (nrzeroes >= 16) {
            for (int nrmarker = 1; nrmarker <= (nrzeroes >> 4); ++nrmarker) {
                writeJpegBits(M16zeroes);
            }
            nrzeroes &= 0xF;
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

 // To get a header file for this, either cut and paste the header,
 // or create jo_jpeg.h, #define JO_JPEG_HEADER_FILE_ONLY, and
 // then include jo_jpeg.c from it.

 // Returns false on failure
template <std::input_iterator IteratorT>
bool jo_write_jpg(IteratorT& inDCT, std::ofstream& outJpeg,
                  int width, int height, int comp, int quality) {
    if (!width || !height || comp > 4 || comp < 1 || comp == 2) {
        return false;
    }

    const auto writeArr =
        [&outJpeg](const auto& arr, std::size_t startOffset = 0) {
            outJpeg.write(reinterpret_cast<const char*>(arr.data() + startOffset),
                            arr.size() - startOffset);
        };

    outJpeg.put(0xFF);
    writeArr(tbl::head2);

    const auto [actualQuality, subsample] = estimQuality(quality);

    std::array<unsigned char, 64> YTable;
    std::array<unsigned char, 64> UVTable;
    for (int i = 0; i < 64; ++i) {
        const auto iZZ = tbl::s_jo_ZigZag[i];
        int yti = (tbl::YQT[i] * actualQuality + 50) / 100;
        YTable[iZZ] = yti < 1 ? 1 : yti > 255 ? 255 : yti;
        int uvti = (tbl::UVQT[i] * actualQuality + 50) / 100;
        UVTable[iZZ] = uvti < 1 ? 1 : uvti > 255 ? 255 : uvti;
    }

    std::array<float, 64> fdtbl_Y;
    std::array<float, 64> fdtbl_UV;
    for (int row = 0, k = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col, ++k) {
            const auto kZZ = tbl::s_jo_ZigZag[k];
            fdtbl_Y[k] = 1 / (YTable[kZZ] * tbl::aasf[row] * tbl::aasf[col]);
            fdtbl_UV[k] = 1 / (UVTable[kZZ] * tbl::aasf[row] * tbl::aasf[col]);
        }
    }

    // Encode 8x8 macroblocks
    int ofsG = comp > 1 ? 1 : 0, ofsB = comp > 1 ? 2 : 0;
    int DCY = 0, DCU = 0, DCV = 0;
    int bitBuf = 0, bitCnt = 0;
    const auto processDU = [&](auto&&... tailArgs) {
        return jo_processDU(inDCT, outJpeg, bitBuf, bitCnt, tailArgs...);
    };

    if (subsample) {
        for (int y = 0; y < height; y += 16) {
            for (int x = 0; x < width; x += 16) {
                std::array<float, 256> Y;
                std::array<float, 256> U;
                std::array<float, 256> V;
                for (int row = y, pos = 0; row < y + 16; ++row) {
                    for (int col = x; col < x + 16; ++col, ++pos) {
                        int prow = row >= height ? height - 1 : row;
                        int pcol = col >= width ? width - 1 : col;
                        int p = prow * width*comp + pcol * comp;
                    }
                }
                DCY = processDU(Y.data() + 0,   16, fdtbl_Y.data(), DCY,
                                tbl::YDC_HT, tbl::YAC_HT);
                DCY = processDU(Y.data() + 8,   16, fdtbl_Y.data(), DCY,
                                tbl::YDC_HT, tbl::YAC_HT);
                DCY = processDU(Y.data() + 128, 16, fdtbl_Y.data(), DCY,
                                tbl::YDC_HT, tbl::YAC_HT);
                DCY = processDU(Y.data() + 136, 16, fdtbl_Y.data(), DCY,
                                tbl::YDC_HT, tbl::YAC_HT);
                // subsample U,V
                std::array<float, 64> subU;
                std::array<float, 64> subV;
                for (int yy = 0, pos = 0; yy < 8; ++yy) {
                    for (int xx = 0; xx < 8; ++xx, ++pos) {
                        const int j = yy * 32 + xx * 2;
                        subU[pos] =
                            (U[j + 0] + U[j + 1] + U[j + 16] + U[j + 17]) * 0.25f;
                        subV[pos] =
                            (V[j + 0] + V[j + 1] + V[j + 16] + V[j + 17]) * 0.25f;
                    }
                }
                DCU = processDU(subU.data(), 8, fdtbl_UV.data(),
                                DCU, tbl::UVDC_HT, tbl::UVAC_HT);
                DCV = processDU(subV.data(), 8, fdtbl_UV.data(),
                                DCV, tbl::UVDC_HT, tbl::UVAC_HT);
            }
        }
    }
    else {
        for (int y = 0; y < height; y += 8) {
            for (int x = 0; x < width; x += 8) {
                std::array<float, 64> Y;
                std::array<float, 64> U;
                std::array<float, 64> V;
                for (int row = y, pos = 0; row < y + 8; ++row) {
                    for (int col = x; col < x + 8; ++col, ++pos) {
                        int prow = row >= height ? height - 1 : row;
                        int pcol = col >= width ? width - 1 : col;
                        int p = prow * width*comp + pcol * comp;
                    }
                }
                DCY = processDU(Y.data(), 8, fdtbl_Y.data(), DCY,
                                tbl::YDC_HT, tbl::YAC_HT);
                DCU = processDU(U.data(), 8, fdtbl_UV.data(), DCU,
                                tbl::UVDC_HT, tbl::UVAC_HT);
                DCV = processDU(V.data(), 8, fdtbl_UV.data(), DCV,
                                tbl::UVDC_HT, tbl::UVAC_HT);
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

#endif // JO_WRITE_JPEG_HPP
