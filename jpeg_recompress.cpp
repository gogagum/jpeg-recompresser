#include <iostream>
#include <string>
#include <fstream>

#include <ael/arithmetic_coder.hpp>
#include <ael/dictionary/adaptive_d_contextual_dictionary_improved.hpp>

#include "lib/file_io.hpp"
#include "lib/nj/nanojpeg.hpp"
#include "lib/magical/process.hpp"
#include "lib/magical/dc_ac.hpp"

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

        auto buff = jrec::io::readWholeFile(inJpeg);

        std::vector<int> blocks;

        auto blocksInserter = std::back_inserter(blocks);
        njDecode(blocksInserter, buff.data(), buff.size());

        int width = nj.getWidth();
        int height = nj.getHeight();
        int ncomp = nj.ncomp;

        auto [dc, ac] = DCAC::separate(blocks);

        auto [acOffset, acProcessed] = Process<63>::process(std::move(ac));

        jrec::io::writeT(outCompressed, imageQuality);
        jrec::io::writeT(outCompressed, width);
        jrec::io::writeT(outCompressed, height);
        jrec::io::writeT(outCompressed, ncomp);

        using Flow = ga::fl::IntegerWordFlow<int, 0, 16>;
        using Word = ga::w::IntegerWord<int, 0, 16>;
        using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
        using Coder = ga::ArithmeticCoder<Flow, Dict>;

        auto dcCoder = Coder(Flow(std::move(dc)));
        auto dcEncoded = dcCoder.encode();

        auto acCoder = Coder(Flow(std::move(acProcessed)));
        auto acEncoded = acCoder.encode();

        jrec::io::writeT<int>(outCompressed, dcEncoded.bytesSize());
        outCompressed.write(reinterpret_cast<const char*>(dcEncoded.data()), dcEncoded.bytesSize());
        jrec::io::writeT<int>(outCompressed, acEncoded.bytesSize());
        jrec::io::writeT(outCompressed, acOffset);
        outCompressed.write(reinterpret_cast<const char*>(acEncoded.data()), acEncoded.bytesSize());

    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
