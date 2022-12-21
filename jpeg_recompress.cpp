#include <iostream>
#include <string>
#include <fstream>

#include "archiever/include/flow/int_range_word_flow.hpp"
#include "archiever/include/arithmetic_coder.hpp"
#include "archiever/include/dictionary/adaptive_dictionary.hpp"
#include "archiever/include/dictionary/static_dictionary.hpp"

#include "lib/file_io.hpp"
#include "lib/nj/nanojpeg.h"

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

        //auto [minBlock, maxBlock] = std::minmax(blocks.begin(), blocks.end());

        using Flow = ga::fl::IntegerWordFlow<int, -128, 127>;
        using Word = ga::w::IntegerWord<int, -128, 127>;
        using Dict = ga::dict::StaticDictionary<Word>;
        using Coder = ga::ArithmeticCoder<Flow, Dict>;


        auto flow = Flow(std::move(blocks));
        auto coder = Coder(std::move(flow));
        auto encoded = coder.encode();

        jrec::io::writeT(outCompressed, imageQuality);
        jrec::io::writeT(outCompressed, width);
        jrec::io::writeT(outCompressed, height);
        jrec::io::writeT(outCompressed, ncomp);

        outCompressed.write(reinterpret_cast<const char*>(encoded.data()), encoded.bytesSize());

    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
