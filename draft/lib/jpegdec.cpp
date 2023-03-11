// NanoJPEG -- KeyJ's Tiny Baseline JPEG Decoder
// version 1.3.5 (2016-11-14)
// Copyright (c) 2009-2016 Martin J. Fiedler <martin.fiedler@gmx.net>
// published under the terms of the MIT license
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

///////////////////////////////////////////////////////////////////////////////
// DOCUMENTATION SECTION                                                     //
// read this if you want to know what this is all about                      //
///////////////////////////////////////////////////////////////////////////////

// INTRODUCTION
// ============
//
// This is a minimal decoder for baseline JPEG images. It accepts memory dumps
// of JPEG files as input and generates either 8-bit grayscale or packed 24-bit
// RGB images as output. It does not parse JFIF or Exif headers; all JPEG files
// are assumed to be either grayscale or YCbCr. CMYK or other color spaces are
// not supported. All YCbCr subsampling schemes with power-of-two ratios are
// supported, as are restart intervals. Progressive or lossless JPEG is not
// supported.
// Summed up, NanoJPEG should be able to decode all images from digital cameras
// and most common forms of other non-progressive JPEG images.
// The decoder is not optimized for speed, it's optimized for simplicity and
// small code. Image quality should be at a reasonable level. A bicubic chroma
// upsampling filter ensures that subsampled YCbCr images are rendered in
// decent quality. The decoder is not meant to deal with broken JPEG files in
// a graceful manner; if anything is wrong with the bitstream, decoding will
// simply fail.
// The code should work with every modern C compiler without problems and
// should not emit any warnings. It uses only (at least) 32-bit integer
// arithmetic and is supposed to be endianness independent and 64-bit clean.
// However, it is not thread-safe.


// COMPILE-TIME CONFIGURATION
// ==========================
//
// The following aspects of NanoJPEG can be controlled with preprocessor
// defines:
//
// NJ_CHROMA_FILTER=1      = Use the bicubic chroma upsampling filter
//                           (default).
// NJ_CHROMA_FILTER=0      = Use simple pixel repetition for chroma upsampling
//                           (bad quality, but faster and less code).


// API
// ===
//
// For API documentation, read the "header section" below.

#include "dec/nanojpeg.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <iomanip>

std::vector<char> readFileBuff(const std::string& filename) {
    std::ifstream inJpeg(filename, std::ios::binary);

    // Stop eating new lines in binary mode!!!
    inJpeg.unsetf(std::ios::skipws);

    // get its size:
    std::streampos fileSize;

    inJpeg.seekg(0, std::ios::end);
    fileSize = inJpeg.tellg();
    inJpeg.seekg(0, std::ios::beg);

    // reserve capacity
    std::vector<char> buff;
    buff.reserve(fileSize);

    // read the data:
    buff.insert(buff.begin(),
               std::istream_iterator<char>(inJpeg),
               std::istream_iterator<char>());
    return buff;
}

int main(int argc, char* argv[])
{

    int size;
    if (argc < 4) {
        std::cout << "Usage: " << argv[0]
                  << " <input.jpg> dct_dump.txt dim.txt quality_of_the_image"
                  << std::endl;
        return 2;
    }

    auto buff = readFileBuff(argv[1]);

    std::ofstream outBlocks;
    outBlocks.open(argv[2], std::ios::out | std::ios::trunc);

    std::vector<int> blocks;

    auto outBlocksIter = std::back_inserter(blocks);

    if (njDecode(outBlocksIter, buff.data(), buff.size())) {
        std::cout << "Error decoding the input file." << std::endl;
        return 1;
    }

    //std::cout << blocks.size() << std::endl;

    for (std::size_t i = 0; i * 64 < blocks.size(); ++i) {
        for (std::size_t j = 0; j < 8; ++j) {
            for (std::size_t k = 0; k < 8; ++k) {
                outBlocks << std::setw(4) << blocks[i * 64 + j * 8 + k];
            }
            outBlocks << std::endl;
        }
        outBlocks << std::endl;
    }

    nj = nj_context_t{};

    auto headerBuff = readFileBuff("headers.bin");

    if (njDecodeHeader(headerBuff.data(), headerBuff.size())) {
        std::cout << "Error decoding header." << std::endl;
        return 1;
    }

    int w = nj.getWidth();
    int h = nj.getHeight();
    std::cout << w << " " << h;
    std::ofstream dim;
    dim.open (argv[3]);
    dim << w << " " << h << " "<< argv[4];
    dim.close();

    return 0;
}
