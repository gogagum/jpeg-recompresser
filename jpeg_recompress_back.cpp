#include <iostream>
#include <string>
#include <fstream>

#include "archiever/include/word/int_range_word.hpp"
#include "archiever/include/dictionary/adaptive_dictionary.hpp"
#include "archiever/include/arithmetic_decoder.hpp"
#include "archiever/include/arithmetic_decoder_decoded.hpp"

#include "lib/file_io.hpp"
#include "lib/jo/jo_write_jpeg.hpp"
#include "lib/magical/process_back.hpp"

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
        int seqOffset = jrec::io::readT<int>(inCompressed);

        std::uint16_t numBits = jrec::io::deserializeNumBits(inCompressed);

        inCompressed.seekg(0, std::ios::beg);

        auto inBuff = jrec::io::readFileBuff(inCompressed);

        std::size_t offset = 5 * sizeof(int) + sizeof(std::uint16_t) /* numBits */;

        std::vector<std::byte> inBufCut;
        inBufCut.resize(inBuff.size() - offset);
        std::memcpy(inBufCut.data(), inBuff.data() + offset, inBufCut.size());

        std::vector<int> blocks;

        if (numBits == 6) {
            using Word = ga::w::IntegerWord<int, 0, 6>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Decoder = ga::ArithmeticDecoder<Word, Dict>;
            auto decoded = ga::ArithmeticDecoderDecoded(std::move(inBufCut));
            auto decoder = Decoder(std::move(decoded));
            auto syms = decoder.decode().syms;
            for (auto w: syms) {
                blocks.push_back(w.getValue());
            }
        } else if (numBits == 7) {
            using Word = ga::w::IntegerWord<int, 0, 7>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Decoder = ga::ArithmeticDecoder<Word, Dict>;
            auto decoded = ga::ArithmeticDecoderDecoded(std::move(inBufCut));
            auto decoder = Decoder(std::move(decoded));
            auto syms = decoder.decode().syms;
            for (auto w: syms) {
                blocks.push_back(w.getValue());
            }
        } else if (numBits == 8) {
            using Word = ga::w::IntegerWord<int, 0, 8>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Decoder = ga::ArithmeticDecoder<Word, Dict>;
            auto decoded = ga::ArithmeticDecoderDecoded(std::move(inBufCut));
            auto decoder = Decoder(std::move(decoded));
            auto syms = decoder.decode().syms;
            for (auto w: syms) {
                blocks.push_back(w.getValue());
            }
        } else {
            using Word = ga::w::IntegerWord<int, 0, 16>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Decoder = ga::ArithmeticDecoder<Word, Dict>;
            auto decoded = ga::ArithmeticDecoderDecoded(std::move(inBufCut));
            auto decoder = Decoder(std::move(decoded));
            auto syms = decoder.decode().syms;
            for (auto w: syms) {
                blocks.push_back(w.getValue());
            }
        }

        blocks = ProcessBack::process(std::move(blocks), seqOffset);
        auto blocksIter = blocks.begin();

        jo_write_jpg(blocksIter, outJpeg, width, height, ncomp, imageQuality);
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
