#include <iostream>
#include <string>
#include <fstream>

#include "archiever/include/flow/int_range_word_flow.hpp"
#include "archiever/include/arithmetic_coder.hpp"
#include "archiever/include/dictionary/adaptive_dictionary.hpp"
#include "archiever/include/dictionary/static_dictionary.hpp"

#include "lib/file_io.hpp"
#include "lib/nj/nanojpeg.h"
#include "lib/magical/process.hpp"

////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <input-jpeg> <out-binary-file> <quality>" << std::endl;
        return 0;
    }

    std::string inFileName = argv[1];
    std::string outFileName = argv[2];
    int imageQuality = std::stoi(argv[3]);

    try {
        auto inJpeg = jrec::io::openInputBinFile(inFileName);
        auto outCompressed = jrec::io::openOutPutBinFile(outFileName);

        auto buff = jrec::io::readFileBuff(inJpeg);

        std::vector<int> blocks;

        {
            auto blocksInserter = std::back_inserter(blocks);
            if (njDecode(blocksInserter, buff.data(), buff.size())) {
                throw std::runtime_error("Error decoding the input file.");
            }
        }

        int width = nj.getWidth();
        int height = nj.getHeight();
        int ncomp = nj.ncomp;

        auto process = Process(blocks);
        auto [offset, blocksProcessed] = process.process();

        for (auto comprIt = blocksProcessed.begin(); comprIt < blocksProcessed.begin() + 100; ++comprIt) {
            std::cout << *comprIt << " ";
        }

        std::cout << std::endl;
        std::cout << offset << std::endl;

        jrec::io::writeT(outCompressed, imageQuality);
        jrec::io::writeT(outCompressed, width);
        jrec::io::writeT(outCompressed, height);
        jrec::io::writeT(outCompressed, ncomp);
        jrec::io::writeT(outCompressed, offset);

        int maxBlock = *std::max_element(blocksProcessed.begin(), blocksProcessed.end());


        if (maxBlock < 64) {
            using Flow = ga::fl::IntegerWordFlow<int, 0, 6>;
            using Word = ga::w::IntegerWord<int, 0, 6>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Coder = ga::ArithmeticCoder<Flow, Dict>;

            auto flow = Flow(std::move(blocksProcessed));
            auto coder = Coder(std::move(flow));
            auto encoded = coder.encode();

            outCompressed.write(reinterpret_cast<const char*>(encoded.data()), encoded.bytesSize());
        } else if (maxBlock < 128) {
            using Flow = ga::fl::IntegerWordFlow<int, 0, 7>;
            using Word = ga::w::IntegerWord<int, 0, 7>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Coder = ga::ArithmeticCoder<Flow, Dict>;

            auto flow = Flow(std::move(blocksProcessed));
            auto coder = Coder(std::move(flow));
            auto encoded = coder.encode();

            outCompressed.write(reinterpret_cast<const char*>(encoded.data()), encoded.bytesSize());
        } else if (maxBlock < 256) {
            using Flow = ga::fl::IntegerWordFlow<int, 0, 8>;
            using Word = ga::w::IntegerWord<int, 0, 8>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Coder = ga::ArithmeticCoder<Flow, Dict>;

            auto flow = Flow(std::move(blocksProcessed));
            auto coder = Coder(std::move(flow));
            auto encoded = coder.encode();

            outCompressed.write(reinterpret_cast<const char*>(encoded.data()), encoded.bytesSize());
        } else {
            using Flow = ga::fl::IntegerWordFlow<int, 0, 16>;
            using Word = ga::w::IntegerWord<int, 0, 16>;
            using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
            using Coder = ga::ArithmeticCoder<Flow, Dict>;

            auto flow = Flow(std::move(blocksProcessed));
            auto coder = Coder(std::move(flow));
            auto encoded = coder.encode();

            outCompressed.write(reinterpret_cast<const char*>(encoded.data()), encoded.bytesSize());
        }

    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
