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

#include "enc/jo_write_jpeg.hpp"

#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>



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

