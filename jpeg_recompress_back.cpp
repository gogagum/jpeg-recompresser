#include <iostream>
#include <string>
#include <fstream>

#include "archiever/include/word/int_range_word.hpp"
#include "archiever/include/dictionary/adaptive_dictionary.hpp"
#include "archiever/include/arithmetic_decoder.hpp"
#include "archiever/include/arithmetic_decoder_decoded.hpp"

#include "lib/file_io.hpp"
#include "lib/jo/jo_write_jpeg.hpp"

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input-jpeg> <out-binary-file>" << std::endl;
        return 0;
    }

    std::string inFileName = argv[1];
    std::string outFileName = argv[2];

    try {
        auto inCompressed = jrec::io::openInputBinFile(inFileName);
        auto outJpeg = jrec::io::openOutPutBinFile(outFileName);

        int imageQuality = jrec::io::readT<int>(inCompressed);
        int width = jrec::io::readT<int>(inCompressed);
        int height = jrec::io::readT<int>(inCompressed);
        int ncomp = jrec::io::readT<int>(inCompressed);
        int minBlk = jrec::io::readT<int>(inCompressed);

        std::uint16_t numBits = jrec::io::readT<std::uint16_t>(inCompressed);

        inCompressed.seekg(0, std::ios::beg);

        auto inBuff = jrec::io::readFileBuff(inCompressed);

        std::size_t offset = 5 * sizeof(int) + sizeof(std::uint16_t) /* numBits */;

        std::vector<std::byte> inBufCut;
        inBufCut.resize(inBuff.size() - offset);
        std::memcpy(inBufCut.data(), inBuff.data() + offset, inBufCut.size());

        using Word = ga::w::IntegerWord<int, 0, 7>;
        using Dict = ga::dict::AdaptiveDictionary<Word>;
        using Decoder = ga::ArithmeticDecoder<Word, Dict>;

        auto decoded = ga::ArithmeticDecoderDecoded(std::move(inBufCut));

        auto decoder = Decoder(std::move(decoded));
        auto ret = decoder.decode();

        std::vector<int> blocks;

        for (auto w: ret.syms) {
            blocks.push_back(w.getValue());
            *blocks.rbegin() += minBlk;
        }

        auto blocksIter = blocks.begin();

        jo_write_jpg(blocksIter, outJpeg, width, height, ncomp, imageQuality);
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
