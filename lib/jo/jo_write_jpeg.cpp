#include <fstream>

#include "jo_write_jpeg.hpp"

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

void jo_DCT(float &d0, float &d1, float &d2, float &d3,
            float &d4, float &d5, float &d6, float &d7) {
    float tmp0 = d0 + d7;
    float tmp7 = d0 - d7;
    float tmp1 = d1 + d6;
    float tmp6 = d1 - d6;
    float tmp2 = d2 + d5;
    float tmp5 = d2 - d5;
    float tmp3 = d3 + d4;
    float tmp4 = d3 - d4;

    // Even part (phase 2)
    float tmp10 = tmp0 + tmp3;
    float tmp13 = tmp0 - tmp3;
    float tmp11 = tmp1 + tmp2;
    float tmp12 = tmp1 - tmp2;

    // phase 3
    d0 = tmp10 + tmp11;
    d4 = tmp10 - tmp11;

    float z1 = (tmp12 + tmp13) * 0.707106781f; // c4

    // phase 5
    d2 = tmp13 + z1;
    d6 = tmp13 - z1;

    // Odd part (phase 2)
    tmp10 = tmp4 + tmp5;
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    // The rotator is modified from fig 4-8 to avoid extra negations.
    float z5 = (tmp10 - tmp12) * 0.382683433f; // c6
    float z2 = tmp10 * 0.541196100f + z5;      // c2-c6
    float z4 = tmp12 * 1.306562965f + z5;      // c2+c6
    float z3 = tmp11 * 0.707106781f;           // c4

     // phase 5
    float z11 = tmp7 + z3;
    float z13 = tmp7 - z3;

    // phase 6
    d5 = z13 + z2;
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

