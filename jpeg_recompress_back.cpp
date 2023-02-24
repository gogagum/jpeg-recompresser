#include <iostream>
#include <string>
#include <fstream>

#include <ael/arithmetic_decoder.hpp>
#include <ael/dictionary/adaptive_d_contextual_dictionary_improved.hpp>

#include "lib/file_io.hpp"
#include "lib/jo/jo_write_jpeg.hpp"
#include "lib/magical/process_back.hpp"
#include "lib/magical/dc_ac.hpp"

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

        //std::uint16_t numBits = jrec::io::deserializeNumBits(inCompressed);

        using Word = ga::w::IntegerWord<int, 0, 16>;
        using Dict = ga::dict::AdaptiveDictionary<Word, 4>;
        using Decoder = ga::ArithmeticDecoder<Word, Dict>;

        int acSize = jrec::io::readT<int>(inCompressed);
        auto acBuff = jrec::io::readFileBuff(inCompressed, acSize);
        auto acDecoder = Decoder(ga::ArithmeticDecoderDecoded(std::move(acBuff)));

        auto acSyms = acDecoder.decode().syms;
        std::vector<int> acProcessed;
        for (auto sym: acSyms) {
            acProcessed.push_back(sym.getValue());
        }

        auto ac = ProcessBack::process(std::move(acProcessed), seqOffset, 63);

        int dcSize = jrec::io::readT<int>(inCompressed);
        auto dcBuff = jrec::io::readFileBuff(inCompressed, dcSize);
        auto dcDecoder = Decoder(ga::ArithmeticDecoderDecoded(std::move(dcBuff)));

        auto dcSyms = dcDecoder.decode().syms;
        std::vector<int> dc;
        for (auto sym: dcSyms) {
            dc.push_back(sym.getValue());
        }

        auto acdc = DCAC::join(dc, ac);
        auto iter = acdc.begin();
        jo_write_jpg(iter, outJpeg, width, height, ncomp, imageQuality);
    } catch (std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        return 1;
    }

    return 0;
}
