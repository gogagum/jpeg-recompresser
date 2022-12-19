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

#include "jo_include_jpeg.hpp"
#include "tables.hpp"


#ifndef JO_JPEG_HEADER_FILE_ONLY

#include <iterator>
#include <iostream>
#include <fstream>
#include <vector>

void jo_writeBits(std::ofstream& outJpeg, int &bitBuf, int &bitCnt, const unsigned short *bs) {
    bitCnt += bs[1];
    bitBuf |= bs[0] << (24 - bitCnt);
    while (bitCnt >= 8) {
        unsigned char c = (bitBuf >> 16) & 255;
        outJpeg.put(c);
        if (c == 255) {
            outJpeg.put(0);
        }
        bitBuf <<= 8;
        bitCnt -= 8;
    }
}

static void
jo_DCT(float &d0, float &d1, float &d2, float &d3, float &d4, float &d5, float &d6, float &d7) {
    const float tmp0 = d0 + d7;
    const float tmp7 = d0 - d7;
    const float tmp1 = d1 + d6;
    const float tmp6 = d1 - d6;
    const float tmp2 = d2 + d5;
    const float tmp5 = d2 - d5;
    const float tmp3 = d3 + d4;
    const float tmp4 = d3 - d4;

    // Even part
    float tmp10 = tmp0 + tmp3;	// phase 2
    float tmp13 = tmp0 - tmp3;
    float tmp11 = tmp1 + tmp2;
    float tmp12 = tmp1 - tmp2;

    d0 = tmp10 + tmp11; 		// phase 3
    d4 = tmp10 - tmp11;

    const float z1 = (tmp12 + tmp13) * 0.707106781f; // c4
    d2 = tmp13 + z1; 		// phase 5
    d6 = tmp13 - z1;

    // Odd part
    tmp10 = tmp4 + tmp5; 		// phase 2
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    // The rotator is modified from fig 4-8 to avoid extra negations.
    const float z5 = (tmp10 - tmp12) * 0.382683433f; // c6
    const float z2 = tmp10 * 0.541196100f + z5; // c2-c6
    const float z4 = tmp12 * 1.306562965f + z5; // c2+c6
    const float z3 = tmp11 * 0.707106781f; // c4

    const float z11 = tmp7 + z3;		// phase 5
    const float z13 = tmp7 - z3;

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

bool
jo_write_headers(std::ofstream& outBinHeaders, const void *data,
                 int width, int height, int comp, int quality) {
    if (!data || !width || !height || comp > 4 || comp < 1 || comp == 2) {
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

    // Write Headers
    const auto writeBinHeaders = [&outBinHeaders](const auto* ptr, std::size_t size) {
        outBinHeaders.write(reinterpret_cast<const char*>(ptr), size);
    };

    writeBinHeaders(tbl::head0, sizeof(tbl::head0));
    writeBinHeaders(YTable, sizeof(YTable));
    outBinHeaders.put(1);
    writeBinHeaders(UVTable, sizeof(UVTable));
    unsigned char h1 = height >> 8;
    unsigned char h2 = height & 0xFF;
    unsigned char w1 = width >> 8;
    unsigned char w2 = width & 0xFF;
    const unsigned char head1[] = {
        0xFF, 0xC0, 0, 0x11, 8, h1, h2, w1, w2, 3, 1,
        (unsigned char) (subsample ? 0x22 : 0x11), 0,
        2, 0x11, 1, 3, 0x11, 1, 0xFF, 0xC4, 0x01, 0xA2, 0
    };
    writeBinHeaders(head1, sizeof(head1));
    writeBinHeaders(tbl::std_dc_luminance_nrcodes + 1, sizeof(tbl::std_dc_luminance_nrcodes) - 1);
    writeBinHeaders(tbl::std_dc_luminance_values, sizeof(tbl::std_dc_luminance_values));
    outBinHeaders.put(0x10); // HTYACinfo
    writeBinHeaders(tbl::std_ac_luminance_nrcodes + 1, sizeof(tbl::std_ac_luminance_nrcodes) - 1);
    writeBinHeaders(tbl::std_ac_luminance_values, sizeof(tbl::std_ac_luminance_values));
    outBinHeaders.put(1); // HTUDCinfo
    writeBinHeaders(tbl::std_dc_chrominance_nrcodes + 1, sizeof(tbl::std_dc_chrominance_nrcodes) - 1);
    writeBinHeaders(tbl::std_dc_chrominance_values, sizeof(tbl::std_dc_chrominance_values));
    outBinHeaders.put(0x11); // HTUACinfo
    writeBinHeaders(tbl::std_ac_chrominance_nrcodes + 1, sizeof(tbl::std_ac_chrominance_nrcodes) - 1);
    writeBinHeaders(tbl::std_ac_chrominance_values, sizeof(tbl::std_ac_chrominance_values));
    writeBinHeaders(tbl::head2, sizeof(tbl::head2));
    return true;
}


int main(int argc, char* argv[]) {
    std::ifstream indct;
    indct.open(argv[1]);
    int i;
    char *filename;
    const void *data;
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    int comp = atoi(argv[4]);
    int quality = atoi(argv[5]);

    auto buffer = std::vector<unsigned char>(comp * width * height * sizeof(unsigned char));

    std::ofstream outJpeg;
    outJpeg.open(argv[6], std::ios::binary | std::ios::out);

    jo_write_jpg(std::istream_iterator<int>(indct), outJpeg, buffer.data(), width, height, comp, quality);

    std::ofstream outBinHeaders;
    outBinHeaders.open("headers.bin", std::ios::binary | std::ios::out);

    jo_write_headers(outBinHeaders, buffer.data(), width, height, comp, quality);

}

#endif
