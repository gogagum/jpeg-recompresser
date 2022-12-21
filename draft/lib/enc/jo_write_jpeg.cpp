#include <fstream>
#include <iostream>

#include "jo_write_jpeg.hpp"
#include "tables.hpp"

void jo_writeBits(std::ofstream& outFile, int &bitBuf, int &bitCnt,
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

void jo_DCT(float &d0, float &d1, float &d2, float &d3, float &d4,
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

void jo_calcBits(int val, unsigned short bits[2]) {
    int tmp1 = val < 0 ? -val : val;
    val = val < 0 ? val - 1 : val;
    bits[1] = 1;
    while (tmp1 >>= 1) {
        ++bits[1];
    }
    bits[0] = val & ((1 << bits[1]) - 1);
}

QualityEstimation estimQuality(int quality) {
    quality = quality ? quality : 90;
    bool subsample = (quality <= 90) ? 1 : 0;
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
        0xFF, 0xC0, 0x00, 0x11, 0x08, h0, h1, w0, w1, 0x03, 0x01,
        (unsigned char) (subsample ? 0x22 : 0x11), 0x00,
        0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xFF, 0xC4, 0x01, 0xA2, 0x00
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
    outBinary.put(0x01); // HTUDCinfo
    writeArr(tbl::std_dc_chrominance_nrcodes, 1);
    writeArr(tbl::std_dc_chrominance_values);
    outBinary.put(0x11); // HTUACinfo
    writeArr(tbl::std_ac_chrominance_nrcodes, 1);
    writeArr(tbl::std_ac_chrominance_values);
    return true;
}

